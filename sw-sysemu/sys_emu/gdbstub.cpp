#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "emu_gio.h"
#include "gdbstub.h"
#include "emu.h"

#define ARRAY_SIZE(x)           (sizeof(x) / sizeof(*x))

#define GDBSTUB_DEFAULT_PORT    1337
#define GDBSTUB_MAX_PACKET_SIZE 2048

#define RSP_START_TOKEN     '$'
#define RSP_END_TOKEN       '#'
#define RSP_ESCAPE_TOKEN    '}'
#define RSP_ESCAPE_XOR      0x20u
#define RSP_RUNLENGTH_TOKEN '*'
#define RSP_PACKET_ACK      '+'
#define RSP_PACKET_NACK     '-'

#define THREAD_ID_ALL_THREADS -1

struct gdbstub_target_description {
    const char *annex;
    const char *data;
};

static const char *gdbstub_target_xml =
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target>"
    "    <architecture>riscv:rv64</architecture>"
    "    <feature name=\"org.gnu.gdb.riscv.fpu\"/>"
    "    <feature name=\"org.gnu.gdb.riscv.csr\"/>"
    "</target>";

static const gdbstub_target_description gdbstub_target_descs[] = {
    {"target.xml", gdbstub_target_xml}
};

static enum gdbstub_status g_status = GDBSTUB_STATUS_NOT_INITIALIZED;
static int g_listen_fd = -1;
static int g_client_fd = -1;
static int g_cur_general_thread = 1;

/** Helper routines ***/

static inline uint64_t bswap64(uint64_t val)
{
    return __builtin_bswap64(val);
}

static inline uint64_t bswap32(uint64_t val)
{
    return __builtin_bswap32(val);
}

static inline int from_hex(char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))      return ch - 'a' + 10;
    else if ((ch >= '0') && (ch <= '9')) return ch - '0';
    else if ((ch >= 'A') && (ch <= 'F')) return ch - 'A' + 10;
    return -1;
}

static inline unsigned int to_hex(int digit)
{
    const char hex_digits[] = "0123456789abcdef";
    return hex_digits[digit & 0xF];
}

/* writes 2*len+1 bytes in buf */
static void memtohex(char *buf, const uint8_t *mem, int len)
{
    for (int i = 0; i < len; i++) {
        *buf++ = to_hex(mem[i] >> 4);
        *buf++ = to_hex(mem[i] & 0xf);
    }
    *buf = '\0';
}

static void hextomem(uint8_t *mem, const char *buf, int len)
{
    for (int i = 0; i < len; i++) {
        mem[i] = (from_hex(buf[0]) << 4) | from_hex(buf[1]);
        buf += 2;
    }
}

static void hextostr(char *str, const char *hex)
{
    while (*hex && *(hex + 1)) {
        *str++ = (from_hex(hex[0]) << 4) | from_hex(hex[1]);
        hex += 2;
    }
    *str = '\0';
}

static inline uint64_t hextou64(const char *buf)
{
    return bswap64(strtoull(buf, NULL, 16));
}

static int strsplit(char *str, const char *delimiters, char *tokens[], int max_tokens)
{
    int n = 0;
    char *tok = strtok(str, delimiters);

    while ((tok != NULL) && (n < max_tokens)) {
        tokens[n++] = tok;
        tok = strtok(NULL, delimiters);
    }

    return n;
}

/** Target platform hooks ***/

static inline unsigned target_num_threads()
{
    return EMU_NUM_THREADS;
}

static bool target_read_memory(uint64_t addr, uint8_t *buffer, uint64_t size)
{
    try {
        bemu::memory.read(addr, size, buffer);
        return true;
    } catch (...) {
        LOG_NOTHREAD(INFO, "%s", "GDB stub: read memory exception");
        return false;
    }
}

static bool target_write_memory(uint64_t addr, const uint8_t *buffer, uint64_t size)
{
    try {
        bemu::memory.write(addr, size, buffer);
        return true;
    } catch (...) {
        LOG_NOTHREAD(INFO, "%s", "GDB stub: write memory exception");
        return false;
    }
}

static uint64_t target_read_register(int thread, int reg)
{
    return sys_emu::thread_get_reg(thread - 1, reg);
}

static void target_write_register(int thread, int reg, uint64_t data)
{
    sys_emu::thread_set_reg(thread - 1, reg, data);
}

static uint32_t target_read_fregister(int thread, int reg)
{
    return sys_emu::thread_get_freg(thread - 1, reg);
}

static void target_write_fregister(int thread, int reg, uint32_t data)
{
    sys_emu::thread_set_freg(thread - 1, reg, data);
}

static uint64_t target_read_pc(int thread)
{
    return sys_emu::thread_get_pc(thread - 1);
}

static void target_write_pc(int thread, uint64_t data)
{
    sys_emu::thread_set_pc(thread - 1, data);
}

static uint64_t target_read_csr(int thread, int csr)
{
    try {
        return sys_emu::thread_get_csr(thread - 1, csr);
    } catch (...) {
        return 0;
    }
}

static void target_write_csr(int thread, int csr, uint64_t data)
{
    try {
        sys_emu::thread_set_csr(thread - 1, csr, data);
    } catch (...) {
    }
}

static void target_step(int thread)
{
    sys_emu::thread_set_single_step(thread - 1);
    sys_emu::thread_set_running(thread - 1);
}

static void target_continue(int thread)
{
    sys_emu::thread_set_running(thread - 1);
}

static void target_breakpoint_insert(uint64_t addr)
{
    sys_emu::breakpoint_insert(addr);
}

static void target_breakpoint_remove(uint64_t addr)
{
    sys_emu::breakpoint_remove(addr);
}

static void target_remote_command(char *cmd)
{
    char *tokens[2];
    int ntokens = strsplit(cmd, " ", tokens, ARRAY_SIZE(tokens));

    LOG_NOTHREAD(INFO, "GDB stub: remote command: \"%s\"", cmd);

    if (ntokens == 0)
        return;

    if (strcmp(tokens[0], "log") == 0) {
        if (ntokens > 1) {
            if (strcmp(tokens[1], "on") == 0)
                emu::log.setLogLevel(LOG_DEBUG);
            else if (strcmp(tokens[1], "off") == 0)
                emu::log.setLogLevel(LOG_INFO);
        }
    }
}

/** RSP protocol ***/

static inline ssize_t rsp_get_char(char *ch)
{
    return recv(g_client_fd, ch, sizeof(*ch), 0);
}

static inline ssize_t rsp_put_char(char ch)
{
    return send(g_client_fd, &ch, sizeof(ch), 0);
}

static inline ssize_t rsp_put_buffer(const void *buf, size_t len)
{
    return send(g_client_fd, buf, len, 0);
}

static inline int rsp_is_token(int ch)
{
    return (ch == RSP_START_TOKEN  ||
            ch == RSP_END_TOKEN    ||
            ch == RSP_ESCAPE_TOKEN ||
            ch == RSP_RUNLENGTH_TOKEN);
}

static ssize_t rsp_receive_packet(char *packet, unsigned int size)
{
    char chr;
    char upper, lower;
    uint8_t checksum;
    ssize_t ret = 0;
    unsigned int len = 0;
    unsigned int sum = 0;

    /* Wait until the packet start */
    do {
        ret = rsp_get_char(&chr);
        if (ret < 0)
            goto failure;
    } while (chr != RSP_START_TOKEN);

    while (len < size) {
        /* Read one character */
        ret = rsp_get_char(&chr);
        if (ret < 0)
            goto failure;

        /* Found escape character, unescape it */
        if (chr == RSP_ESCAPE_TOKEN) {
            sum += chr;
            ret = rsp_get_char(&chr);
            if (ret < 0)
                goto failure;

            chr ^= RSP_ESCAPE_XOR;
        }

        /* End of packet */
        if (chr == RSP_END_TOKEN)
            break;

        sum += chr;
        packet[len++] = chr;
    }

    /* Read packet checksum */
    ret = rsp_get_char(&upper);
    if (ret < 0)
        goto failure;
    ret = rsp_get_char(&lower);
    if (ret < 0)
        goto failure;

    /* Checksum is mod 256 of sum of all data */
    checksum = from_hex(upper) << 4 | from_hex(lower);
    if ((sum & 0xFF) != checksum) {
        LOG_NOTHREAD(WARN, "GDB stub: read_packet checksum mismatch, "
                           "0x%02X (calc) != 0x%02X (packet)\n",
                     (sum & 0xFF), checksum);
        ret = 0;
        goto failure;
    }

    /* Null terminate our string */
    packet[len] = '\0';
    rsp_put_char(RSP_PACKET_ACK);

    return len;

failure:
    rsp_put_char(RSP_PACKET_NACK);
    return ret;
}

static ssize_t rsp_send_packet_len(const char *packet, size_t len)
{
    char buf[GDBSTUB_MAX_PACKET_SIZE];
    size_t cnt = 0;
    unsigned int sum = 0;

    LOG_NOTHREAD(DEBUG, "GDB stub: send packet: \"%s\"", packet);

    /* Write the start token */
    buf[cnt++] = RSP_START_TOKEN;

    for (size_t i = 0; i < len; i++) {
        char chr = packet[i];

        /* Check for any reserved tokens */
        if (rsp_is_token(chr)) {
            buf[cnt++] = RSP_ESCAPE_TOKEN;
            sum += RSP_ESCAPE_TOKEN;
            chr ^= RSP_ESCAPE_XOR;
        }

        buf[cnt++] = chr;
        sum += chr;
    }

    /* Done with data, now end + checksum (mod 256) */
    sum &= 0xFF;
    buf[cnt++] = RSP_END_TOKEN;
    buf[cnt++] = to_hex((sum >> 4) & 0xF);
    buf[cnt++] = to_hex(sum & 0xF);

    return rsp_put_buffer(buf, cnt);
}

static inline ssize_t rsp_send_packet(const char *packet)
{
    return rsp_send_packet_len(packet, strlen(packet));
}

static int rsp_is_query_packet(const char *p, const char *query, char separator)
{
    unsigned int query_len = strlen(query);
    return strncmp(p + 1, query, query_len) == 0 &&
           (p[query_len + 1] == '\0' || p[query_len + 1] == separator);
}

/** GDB stub routines ***/

static void gdbstub_handle_qsupported(void)
{
    char reply[GDBSTUB_MAX_PACKET_SIZE + 1];

    LOG_NOTHREAD(DEBUG, "GDB stub: %s", "handle qSupported");

    snprintf(reply, sizeof(reply),
         "PacketSize=%x;qXfer:features:read+;qXfer:threads:read+;vContSupported+;hwbreak+",
         GDBSTUB_MAX_PACKET_SIZE);

    rsp_send_packet(reply);
}

static ssize_t gdbstub_qxfer_send_object(const char *object, size_t object_size,
                                         unsigned long offset, unsigned long length)
{
    char resp;
    char reply[GDBSTUB_MAX_PACKET_SIZE + 2];

    if (offset == object_size) { /* Offset at the end, no more data to be read */
        rsp_send_packet("l");
        return 0;
    } else if (offset > object_size) { /* Out of bounds */
        rsp_send_packet("E16"); /* EINVAL */
        return -EINVAL;
    }

    if (offset + length > object_size) {
        length = object_size - offset;
        resp = 'l';
    } else {
        resp = 'm'; /* More data to be read */
    }

    reply[0] = resp;
    strncpy(&reply[1], object + offset, length);
    reply[length + 1] = '\0';
    return rsp_send_packet_len(reply, length + 1);
}

static void gdbstub_handle_qxfer_features_read(const char *annex, const char *offset_length)
{
    const char *annex_data = NULL;
    unsigned long offset = strtoul(offset_length, NULL, 16);
    unsigned long length = strtoul(strchr(offset_length, ',') + 1 , NULL, 16);

    LOG_NOTHREAD(DEBUG, "GDB stub: handle qXfer:features:read annex: %s, offset: 0x%lx, size: 0x%lx",
        annex, offset, length);

    for (unsigned int i = 0; i < ARRAY_SIZE(gdbstub_target_descs); i++) {
        if (strcmp(annex, gdbstub_target_descs[i].annex) == 0) {
            annex_data = gdbstub_target_descs[i].data;
            break;
        }
    }

    if (!annex_data) { /* Annex not found! */
        rsp_send_packet("E00");
        return;
    }

    gdbstub_qxfer_send_object(annex_data, strlen(annex_data), offset, length);
}

static void gdbstub_handle_qxfer_features(char *tokens[], int ntokens)
{
    if (ntokens < 5) {
        rsp_send_packet("");
        return;
    }

    if (strcmp(tokens[2], "read") == 0)
        gdbstub_handle_qxfer_features_read(tokens[3], tokens[4]);
    else
        rsp_send_packet("");
}

static void gdbstub_handle_qxfer_threads(char *tokens[], int ntokens)
{
    unsigned long offset, length;
    /* TODO: Generate the list */
    static const char *thread_list_xml =
        "<?xml version=\"1.0\"?>"
        "<!DOCTYPE threads SYSTEM \"threads.dtd\">"
        "<threads>"
        "    <thread id=\"1\" core=\"0\" name=\"S0:M0:T0\"></thread>"
        "    <thread id=\"2\" core=\"0\" name=\"S0:M0:T1\"></thread>"
        "</threads>";

    if (ntokens < 4) {
        rsp_send_packet("");
        return;
    }

    /* Only read is supported */
    if (strcmp(tokens[2], "read")) {
        rsp_send_packet("");
        return;
    }

    offset = strtoul(tokens[3], NULL, 16);
    length = strtoul(strchr(tokens[3], ',') + 1 , NULL, 16);

    gdbstub_qxfer_send_object(thread_list_xml, strlen(thread_list_xml), offset, length);
}

static void gdbstub_handle_qxfer(char *packet)
{
    char *tokens[5];
    int ntokens = strsplit(packet, ":", tokens, ARRAY_SIZE(tokens));

    if (ntokens < 2) {
        rsp_send_packet("");
        return;
    }

    if (strcmp(tokens[1], "features") == 0)
        gdbstub_handle_qxfer_features(tokens, ntokens);
    else if (strcmp(tokens[1], "threads") == 0)
        gdbstub_handle_qxfer_threads(tokens, ntokens);
    else
        rsp_send_packet("");
}

static void gdbstub_handle_qfthreadinfo(void)
{
    LOG_NOTHREAD(DEBUG, "GDB stub: %s", "handle qfThreadInfo");

    /* TODO: Implement properly. qXfer:threads has preference over this. */
    rsp_send_packet("m0,1,2,3,4,5");
}

static void gdbstub_handle_qsthreadinfo(void)
{
    LOG_NOTHREAD(DEBUG, "GDB stub: %s", "handle qsThreadInfo");

    /* End of list */
    rsp_send_packet("l");
}

static void gdbstub_handle_read_general_registers(void)
{
    char buffer[(32 + 1) * 16 + (32 + 3) * 8 + 1];
    char tmp[16 + 1];
    buffer[0] = '\0';

    /* General purpose registers */
    for (int i = 0; i < 32; i++) {
        uint64_t value = bswap64(target_read_register(g_cur_general_thread, i));
        snprintf(tmp, sizeof(tmp), "%016" PRIx64, value);
        strcat(buffer, tmp);
    }

    /* PC */
    snprintf(tmp, sizeof(tmp), "%016" PRIx64, bswap64(target_read_pc(g_cur_general_thread)));
    strcat(buffer, tmp);

    /* Floating point registers */
    for (int i = 0; i < 32; i++) {
        uint32_t value = bswap32(target_read_fregister(g_cur_general_thread, i));
        snprintf(tmp, sizeof(tmp), "%08" PRIx32, value);
        strcat(buffer, tmp);
    }

    snprintf(tmp, sizeof(tmp), "%08" PRIx32, (uint32_t)target_read_csr(g_cur_general_thread, CSR_FFLAGS));
    strcat(buffer, tmp);
    snprintf(tmp, sizeof(tmp), "%08" PRIx32, (uint32_t)target_read_csr(g_cur_general_thread, CSR_FRM));
    strcat(buffer, tmp);
    snprintf(tmp, sizeof(tmp), "%08" PRIx32, (uint32_t)target_read_csr(g_cur_general_thread, CSR_FCSR));
    strcat(buffer, tmp);

    rsp_send_packet(buffer);
}

static void gdbstub_handle_read_register(const char *packet)
{
    char buffer[32];
    uint64_t reg = strtoul(packet + 1, NULL, 16);

    LOG_NOTHREAD(DEBUG, "GDB stub: read register: %ld", reg);

    if (reg < 32) {
        uint64_t value = bswap64(target_read_register(g_cur_general_thread, reg));
        snprintf(buffer, sizeof(buffer), "%016" PRIx64, value);
    } else if (reg == 32) { /* PC register */
        uint64_t value = bswap64(target_read_pc(g_cur_general_thread));
        snprintf(buffer, sizeof(buffer), "%016" PRIx64, value);
    } else if (reg < 64 + 3) { /* Floating point registers + fflags, frm, fcsr */
        uint32_t value = bswap64(target_read_fregister(g_cur_general_thread, reg - 33));
        if (reg < 65) {
            value = target_read_fregister(g_cur_general_thread, reg - 33);
        } else if (reg == 65) { /* CSR fflags */
            value = target_read_csr(g_cur_general_thread, CSR_FFLAGS);
        } else if (reg == 66) { /* CSR frm */
            value = target_read_csr(g_cur_general_thread, CSR_FRM);
        } else if (reg == 67) { /* CSR fcsr */
            value = target_read_csr(g_cur_general_thread, CSR_FCSR);
        }
        snprintf(buffer, sizeof(buffer), "%08" PRIx32, value);
    } else {
        LOG_NOTHREAD(INFO, "GDB stub: read register: unknown register %ld", reg);
        rsp_send_packet("E00");
        return;
    }

    rsp_send_packet(buffer);
}

static void gdbstub_handle_write_register(const char *packet)
{
    uint64_t reg = strtoul(packet + 1, NULL, 16);
    uint64_t value = hextou64(strchr(packet + 1, '=') + 1);

    LOG_NOTHREAD(DEBUG, "GDB stub: write register: %ld <- %ld", reg, value);

    if (reg < 32) {
        target_write_register(g_cur_general_thread, reg, value);
    } else if (reg == 32) { /* PC register */
        target_write_pc(g_cur_general_thread, value);
    } else if (reg < 65) { /* Floating point registers */
        target_write_fregister(g_cur_general_thread, reg - 33, value);
    } else if (reg == 65) { /* CSR fflags */
        target_write_csr(g_cur_general_thread, CSR_FFLAGS, value);
    } else if (reg == 66) { /* CSR frm */
        target_write_csr(g_cur_general_thread, CSR_FRM, value);
    } else if (reg == 67) { /* CSR fcsr */
        target_write_csr(g_cur_general_thread, CSR_FCSR, value);
    } else {
        LOG_NOTHREAD(INFO, "GDB stub: write register: unknown register %ld", reg);
        rsp_send_packet("E00");
        return;
    }

    rsp_send_packet("OK");
}

static void gdbstub_handle_set_thread(const char *packet)
{
    char op = packet[1];

    LOG_NOTHREAD(DEBUG, "GDB stub: handle set_thread, op: %c", op);

    if (op == 'g') {
        g_cur_general_thread = atoi(&packet[2]);
        /* 0 means pick any thread */
        if (g_cur_general_thread == 0)
            g_cur_general_thread = 1;
    } else if (op == 'c') {
        /* We support vCont, this is deprecated */
        rsp_send_packet("");
        return;
    } else {
        LOG_NOTHREAD(INFO, "GDB stub: handle set_thread: invalid op: %c", op);
        return;
    }

    rsp_send_packet("OK");
}

static void gdbstub_handle_query_thread(void)
{
    /* We support vCont, this is deprecated */
    rsp_send_packet("");
}

static void gdbstub_handle_qrcmd(const char *packet)
{
    char cmd[GDBSTUB_MAX_PACKET_SIZE + 1];
    const char *cmd_hex = strchr(packet, ',');
    if (cmd_hex) {
        hextostr(cmd, cmd_hex + 1);
        target_remote_command(cmd);
        rsp_send_packet("OK");
    } else {
        rsp_send_packet("");
    }
}

static void gdbstub_handle_thread_alive(const char *packet)
{
    int thread_id;
    (void) thread_id;

    LOG_NOTHREAD(DEBUG, "GDB stub: %s", "handle thread alive");

    thread_id = atoi(&packet[1]);
    /* TODO: Properly implement */
    rsp_send_packet("OK");
}

static inline void gdbstub_handle_vcont_action(char action, int thread)
{
    if (action == 's')
        target_step(thread);
    else if (action == 'c')
        target_continue(thread);
}

static void gdbstub_handle_vcont(char *packet)
{
    char *action;

    LOG_NOTHREAD(DEBUG, "GDB stub: %s", "handle vCont");

    /* For each action */
    action = strtok(packet + 5, ";");
    while (action != NULL) {
        int thread = THREAD_ID_ALL_THREADS;
        /* Find optional thread-id */
        char *threadptr = strchr(action, ':');
        if (threadptr) {
            thread = atoi(threadptr + 1);
            *threadptr = '\0';
        }

        LOG_NOTHREAD(DEBUG, "GDB stub: vCont action %s, thread: %d", action, thread);

        if (thread == THREAD_ID_ALL_THREADS) {
            for (unsigned id = 1; id <= target_num_threads(); id++)
                gdbstub_handle_vcont_action(*action, id);
        } else {
            gdbstub_handle_vcont_action(*action, thread);
        }

        action = strtok(NULL, ";");
    }

    /* The response is sent when something (such a breakpoint) happens */
}

static bool parse_breakpoint(char *packet, char *type, uint64_t *addr, uint64_t *kind)
{
    char *tokens[3];
    int ntokens = strsplit(packet, ",", tokens, ARRAY_SIZE(tokens));

    if (ntokens < 3)
        return false;

    *type = tokens[0][1];
    *addr = strtoull(tokens[1], NULL, 16);
    *kind = strtoull(tokens[2], NULL, 16);
    return true;
}

static void gdbstub_handle_breakpoint_insert(char *packet)
{
    char type;
    uint64_t addr, kind;

    if (!parse_breakpoint(packet, &type, &addr, &kind)) {
        LOG_NOTHREAD(INFO, "GDB stub: insert breakpoint: %s", "unknown parameters");
        rsp_send_packet("");
        return;
    }

    LOG_NOTHREAD(DEBUG, "GDB stub: insert breakpoint %c 0x%" PRIx64 " %" PRIu64, type, addr, kind);

    switch (type) {
    case '0': /* Software breakpoint */
        target_breakpoint_insert(addr);
        rsp_send_packet("OK");
        break;
    case '1': /* Hardware breakpoint */
        target_breakpoint_insert(addr);
        rsp_send_packet("OK");
        break;
    case '2': /* Write watchpoint */
        rsp_send_packet("");
        break;
    case '3': /* Read watchpoint */
        rsp_send_packet("");
        break;
    case '4': /* Access watchpoint */
        rsp_send_packet("");
        break;
    }
}

static void gdbstub_handle_breakpoint_remove(char *packet)
{
    char type;
    uint64_t addr, kind;

    if (!parse_breakpoint(packet, &type, &addr, &kind)) {
        LOG_NOTHREAD(INFO, "GDB stub: remove breakpoint: %s", "unknown parameters");
        rsp_send_packet("");
        return;
    }

    LOG_NOTHREAD(DEBUG, "GDB stub: remove breakpoint %c 0x%" PRIx64 " %" PRIu64, type, addr, kind);

    switch (type) {
    case '0': /* Software breakpoint */
        target_breakpoint_remove(addr);
        rsp_send_packet("OK");
        break;
    case '1': /* Hardware breakpoint */
        target_breakpoint_remove(addr);
        rsp_send_packet("OK");
        break;
    case '2': /* Write watchpoint */
        rsp_send_packet("");
        break;
    case '3': /* Read watchpoint */
        rsp_send_packet("");
        break;
    case '4': /* Access watchpoint */
        rsp_send_packet("");
        break;
    }
}

static void gdbstub_handle_read_memory(const char *packet)
{
    char *p;
    uint64_t addr, length;
    uint8_t data[GDBSTUB_MAX_PACKET_SIZE / 2];
    char send[2 * sizeof(data) + 1];

    addr = strtoull(packet + 1, &p, 16);
    if (*p == ',')
        p++;
    length = strtoull(p, NULL, 16);

    LOG_NOTHREAD(DEBUG, "GDB stub: read memory: from 0x%lx, size 0x%lx", addr, length);

    if (!target_read_memory(addr, data, length))
        rsp_send_packet("E01");

    memtohex(send, data, length);

    rsp_send_packet(send);
}

static void gdbstub_handle_write_memory(const char *packet)
{
    char *p;
    uint64_t addr, length;
    uint8_t data[GDBSTUB_MAX_PACKET_SIZE / 2];

    addr = strtoull(packet + 1, &p, 16);
    if (*p == ',')
        p++;
    length = strtoull(p, &p, 16);

    LOG_NOTHREAD(DEBUG, "GDB stub: write memory: from 0x%lx, size 0x%lx", addr, length);

    hextomem(data, p, length);

    if (!target_write_memory(addr, data, length))
        rsp_send_packet("E01");

    rsp_send_packet("OK");
}

static int gdbstub_handle_packet(char *packet)
{
    switch (packet[0]) {
    case '!': /* Enable extended mode */
        rsp_send_packet("OK");
        break;
    case '?': /* Halt reason */
        rsp_send_packet("S05"); /* SIGTRAP */
        break;
    case 'c': /* Continue */
        /* We support vCont, this is deprecated */
        rsp_send_packet("");
        break;
    case 'D': /* Detach */
        rsp_send_packet("OK");
        gdbstub_close_client();
        break;
    case 'g': /* Read general registers */
        gdbstub_handle_read_general_registers();
        break;
    case 'H': /* Set thread for subsequent operations */
        /* GDB always sends 'Hg0', even if we support vCont */
        gdbstub_handle_set_thread(packet);
        break;
    case 'k': /* Kill */
        return -1;
    case 'm': /* Read memory */
        gdbstub_handle_read_memory(packet);
        break;
    case 'M': /* Write memory */
        gdbstub_handle_write_memory(packet);
        break;
    case 'p': /* Read register */
        gdbstub_handle_read_register(packet);
        break;
    case 'P': /* Write register */
        gdbstub_handle_write_register(packet);
        break;
    case 'q': /* General query */
        if (rsp_is_query_packet(packet, "Supported", ':'))
            gdbstub_handle_qsupported();
        else if (rsp_is_query_packet(packet, "Xfer", ':'))
            gdbstub_handle_qxfer(packet);
        else if (rsp_is_query_packet(packet, "Rcmd", ','))
            gdbstub_handle_qrcmd(packet);
        else if (strcmp(packet, "qfThreadInfo") == 0)
            gdbstub_handle_qfthreadinfo();
        else if (strcmp(packet, "qsThreadInfo") == 0)
            gdbstub_handle_qsthreadinfo();
        else if (strcmp(packet, "qAttached") == 0)
            rsp_send_packet("1");
        else if (strcmp(packet, "qC") == 0)
            gdbstub_handle_query_thread();
        else
            rsp_send_packet("");
        break;
    case 'Q': /* General set */
        rsp_send_packet("");
        break;
    case 's': /* Single step */
        /* We support vCont, this is deprecated */
        rsp_send_packet("");
        break;
    case 'T': /* Thread alive */
        gdbstub_handle_thread_alive(packet);
        break;
    case 'v': /* Multi-letter name packet */
        if (strncmp(packet, "vCont", 5) == 0) {
            if (packet[5] == '?')
                rsp_send_packet("vCont;c;C;s;S;r");
            else
                gdbstub_handle_vcont(packet);
        } else {
            rsp_send_packet("");
        }
        break;
    case 'z': /* Remove breakpoint */
        gdbstub_handle_breakpoint_remove(packet);
        break;
    case 'Z': /* Insert breakpoint */
        gdbstub_handle_breakpoint_insert(packet);
        break;
    default:
        LOG_NOTHREAD(DEBUG, "GDB stub: unrecognized command \"%c\"", packet[0]);
        rsp_send_packet("");
        break;
    }

    return 0;
}

static int gdbstub_open_port(unsigned short *port)
{
    struct sockaddr_in sockaddr;
    int fd, ret, opt;
    socklen_t len = sizeof(sockaddr);

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_NOTHREAD(INFO, "GDB stub: socket error: %d", fd);
        return fd;
    }

    /* Allow rapid reuse of this port */
    opt = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
        LOG_NOTHREAD(INFO, "GDB stub: setsockopt(SO_REUSEADDR) error: %d", ret);
        close(fd);
        return ret;
    }

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(GDBSTUB_DEFAULT_PORT);
    sockaddr.sin_addr.s_addr = 0;
    ret = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (ret < 0) {
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(0); /* Automatic port */
        sockaddr.sin_addr.s_addr = 0;
        ret = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
        if (ret < 0) {
            LOG_NOTHREAD(INFO, "GDB stub: bind error: %d", ret);
            close(fd);
            return ret;
        }
    }

    ret = getsockname(fd, (struct sockaddr *)&sockaddr, &len);
    if (ret < 0) {
        LOG_NOTHREAD(INFO, "GDB stub: getsockname error: %d", ret);
        close(fd);
        return ret;
    }

    *port = ntohs(sockaddr.sin_port);

    ret = listen(fd, 1);
    if (ret < 0) {
        LOG_NOTHREAD(INFO, "GDB stub: listen error: %d", ret);
        close(fd);
        return -1;
    }

    return fd;
}

static int gdbstub_accept(int listen_fd)
{
    struct sockaddr_in sockaddr;
    socklen_t len;
    int fd, ret, opt;

    len = sizeof(sockaddr);
    fd = accept(listen_fd, (struct sockaddr *)&sockaddr, &len);
    if (fd < 0) {
        LOG_NOTHREAD(INFO, "GDB stub: accept error: %d", fd);
        return fd;
    }

    /* Disable Nagle - allow small packets to be sent without delay. */
    opt = 1;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    if (ret < 0) {
        LOG_NOTHREAD(INFO, "GDB stub: setsockopt(TCP_NODELAY) error: %d", ret);
        close(fd);
        return ret;
    }

    LOG_NOTHREAD(INFO, "GDB stub: Client %s connected", inet_ntoa(sockaddr.sin_addr));

    return fd;
}

int gdbstub_init()
{
    unsigned short port;

    if (g_status != GDBSTUB_STATUS_NOT_INITIALIZED)
        return -1;

    LOG_NOTHREAD(INFO, "GDB stub: %s", "Initializing...");

    g_listen_fd = gdbstub_open_port(&port);
    if (g_listen_fd < 0) {
        return g_listen_fd;
    }

    LOG_NOTHREAD(INFO, "GDB stub: Listening on port %d ...", port);

    g_status = GDBSTUB_STATUS_WAITING_CLIENT;
    return 0;
}

int gdbstub_accept_client()
{
    if (g_status != GDBSTUB_STATUS_WAITING_CLIENT)
        return -1;

    g_client_fd = gdbstub_accept(g_listen_fd);
    if (g_client_fd < 0)
        return g_client_fd;

    g_status = GDBSTUB_STATUS_RUNNING;
    return 0;
}

int gdbstub_close_client()
{
    if (g_status != GDBSTUB_STATUS_RUNNING)
        return -1;

    close(g_client_fd);
    g_client_fd = -1;

    LOG_NOTHREAD(INFO, "%s", "GDB stub: Client connection closed");

    g_status = GDBSTUB_STATUS_WAITING_CLIENT;
    return 0;
}

void gdbstub_fini()
{
    LOG_NOTHREAD(INFO, "%s", "GDB stub: Finishing...");

    if (g_listen_fd > 0) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }

    gdbstub_close_client();

    g_status = GDBSTUB_STATUS_NOT_INITIALIZED;
}

int gdbstub_io()
{
    ssize_t ret;
    struct pollfd pollfd;
    char packet[GDBSTUB_MAX_PACKET_SIZE + 1];

    if (g_status != GDBSTUB_STATUS_RUNNING)
        return -1;

    memset(&pollfd, 0, sizeof(pollfd));
    pollfd.fd = g_client_fd;
    pollfd.events = POLLIN;

    /* Return immediately */
    ret = poll(&pollfd, 1, 0);
    if (ret <= 0)
        return ret;

    ret = rsp_receive_packet(packet, sizeof(packet) - 1);
    if (ret < 0) {
        LOG_NOTHREAD(WARN, "GDB stub: RSP: error receiving packet: %s",
                     strerror(ret));
        gdbstub_close_client();
        return ret;
    } else if (ret == 0) {
        return 0;
    }

    LOG_NOTHREAD(DEBUG, "GDB stub: recv packet: \"%s\"", packet);

    return gdbstub_handle_packet(packet);
}

void gdbstub_signal(int signal)
{
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "S%02X", signal);
    rsp_send_packet_len(buffer, len);
}

enum gdbstub_status gdbstub_get_status()
{
    return g_status;
}
