/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "gdbstub.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "gdb_target_xml.h"
#include "sys_emu.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

#define GDBSTUB_DEFAULT_PORT    1337
#define GDBSTUB_MAX_PACKET_SIZE 4096

#define RSP_START_TOKEN     '$'
#define RSP_END_TOKEN       '#'
#define RSP_ESCAPE_TOKEN    '}'
#define RSP_ESCAPE_XOR      0x20u
#define RSP_RUNLENGTH_TOKEN '*'
#define RSP_PACKET_ACK      '+'
#define RSP_PACKET_NACK     '-'

#define THREAD_ID_ALL_THREADS -1

static const unsigned target_csr_list[]{
#define CSRDEF(num, lower, upper) num,
#include "csrs.h"
#undef CSRDEF
};

#define NUM_XREGS     32
#define NUM_FREGS     32
#define NUM_CSR_FREGS 3

#define XREGS_START     0
#define XREGS_END       (XREGS_START + NUM_XREGS - 1)
#define PC_REG          (XREGS_END + 1)
#define TP_REG          (XREGS_START + 4)
#define FREGS_START     (PC_REG + 1)
#define FREGS_END       (FREGS_START + NUM_FREGS - 1)
#define CSR_FREGS_START (FREGS_END + 1)
#define CSR_FREGS_END   (CSR_FREGS_START + NUM_CSR_FREGS - 1)
#define CSR_REGS_START  (CSR_FREGS_END + 1)
#define CSR_REGS_END    (CSR_REGS_START + ARRAY_SIZE(target_csr_list) - 1)

#define LOG_GDBSTUB(lvl, fmt, ...) LOG_AGENT(lvl, g_agent, fmt, __VA_ARGS__)

struct gdbstub_target_description {
    const char* annex;
    const char* data;
    size_t      size;
};

static const gdbstub_target_description gdbstub_target_descs[] = {
    {"target.xml", gdb_target_xml, sizeof(gdb_target_xml) - 1},
    {"target_csr.xml", gdb_target_csr_xml, sizeof(gdb_target_csr_xml) - 1}};

static enum gdbstub_status g_status             = GDBSTUB_STATUS_NOT_INITIALIZED;
static int                 g_listen_fd          = -1;
static int                 g_client_fd          = -1;
static int                 g_cur_general_thread = 1;
static char*               g_thread_list_xml    = NULL;
static sys_emu*            g_sys_emu            = nullptr;
static bemu::Noagent       g_agent(nullptr);

/** Helper routines ***/

static inline uint64_t bswap64(uint64_t val)
{
    return __builtin_bswap64(val);
}

static inline uint32_t bswap32(uint64_t val)
{
    return __builtin_bswap32(val);
}

static inline uint8_t from_hex(char ch)
{
    if ((ch >= '0') && (ch <= '9')) {
        return ch - '0';
    }
    if ((ch >= 'a') && (ch <= 'f')) {
        return ch - 'a' + 10;
    }
    if ((ch >= 'A') && (ch <= 'F')) {
        return ch - 'A' + 10;
    }
    return 0xff;
}

static inline unsigned int to_hex(int digit)
{
    const char hex_digits[] = "0123456789abcdef";
    return hex_digits[digit & 0xF];
}

/* writes 2*len+1 bytes in buf */
static void memtohex(char* buf, const uint8_t* mem, int len)
{
    for (int i = 0; i < len; i++) {
        *buf++ = to_hex(mem[i] >> 4);
        *buf++ = to_hex(mem[i] & 0xf);
    }
    *buf = '\0';
}

static void hextomem(uint8_t* mem, const char* buf, int len)
{
    for (int i = 0; i < len; i++) {
        mem[i] = (from_hex(buf[0]) << 4) | from_hex(buf[1]);
        buf += 2;
    }
}

static void hextostr(char* str, const char* hex)
{
    while (*hex && *(hex + 1)) {
        *str++ = (from_hex(hex[0]) << 4) | from_hex(hex[1]);
        hex += 2;
    }
    *str = '\0';
}

static inline void u32_to_hexstr(char* str, uint32_t value)
{
    sprintf(str, "%08" PRIx32, bswap32(value));
}

static inline void u64_to_hexstr(char* str, uint64_t value)
{
    sprintf(str, "%016" PRIx64, bswap64(value));
}

static inline void u64_to_hexstr_raw(char* str, uint64_t value)
{
    sprintf(str, "%016" PRIx64, value);
}

static inline void freg_to_hexstr(char* str, bemu::freg_t freg)
{
    char tmp[8 * 2 + 1];
    str[0] = '\0';

    for (unsigned i = 0; i < bemu::VLEN / 64; i++) {
        u64_to_hexstr(tmp, freg.u64[i]);
        strcat(str, tmp);
    }
}

static inline uint64_t hexstr_to_u64(const char* buf)
{
    return bswap64(strtoull(buf, NULL, 16));
}

static inline uint32_t hexstr_to_u32(const char* buf)
{
    return bswap32(strtoul(buf, NULL, 16));
}

static inline bemu::freg_t hexstr_to_freg(const char* buf)
{
    bemu::freg_t freg;
    char         tmp[8 * 2 + 1];

    for (unsigned i = 0; i < bemu::VLEN / 64; i++) {
        strncpy(tmp, &buf[i * 16], 16);
        freg.u64[i] = hexstr_to_u64(tmp);
    }

    return freg;
}

static inline int strsplit(char* str, const char* delimiters, char* tokens[], int max_tokens)
{
    int   n   = 0;
    char* tok = strtok(str, delimiters);

    while ((tok != NULL) && (n < max_tokens)) {
        tokens[n++] = tok;
        tok         = strtok(NULL, delimiters);
    }

    return n;
}

static inline char* strconcat(char* dst, const char* suffix)
{
    char*  newstr;
    size_t dst_len    = strlen(dst);
    size_t suffix_len = strlen(suffix);
    newstr            = (char*)realloc((void*)dst, dst_len + suffix_len + 1);
    strcat(newstr, suffix);
    return newstr;
}

/** Target platform hooks ***/

/* Conversion from GDB thread ID to SysEMU thread ID (thread index) */
static inline int to_target_thread(int thread_id)
{
    return ((thread_id - 1) == IO_SHIRE_SP_HARTID) ? EMU_IO_SHIRE_SP_THREAD : thread_id - 1;
}

static inline int to_gdb_thread(int thread_id)
{
    if ((thread_id == IO_SHIRE_SP_HARTID) || (thread_id == EMU_IO_SHIRE_SP_THREAD)) {
        return IO_SHIRE_SP_HARTID + 1;
    }
    else {
        return thread_id + 1;
    }
}

static inline unsigned target_num_threads()
{
    return EMU_NUM_THREADS;
}

/* Returns whether the thread is "physically present" */
static inline bool target_thread_exists(int thread)
{
    return g_sys_emu->thread_exists(to_target_thread(thread));
}

static inline bool target_thread_is_alive(int thread)
{
    return !g_sys_emu->thread_is_unavailable(to_target_thread(thread))
        /* && !g_sys_emu->thread_is_running(to_target_thread(thread)) */;
}

static bool target_read_memory(int thread, uint64_t addr, uint8_t* buffer, uint64_t size)
{
    try {
        g_sys_emu->thread_read_memory(to_target_thread(thread), addr, size, buffer);
        return true;
    }
    catch (...) {
        LOG_GDBSTUB(INFO, "%s", "read memory exception");
        return false;
    }
}

static bool target_write_memory(int thread, uint64_t addr, const uint8_t* buffer, uint64_t size)
{
    try {
        g_sys_emu->thread_write_memory(to_target_thread(thread), addr, size, buffer);
        return true;
    }
    catch (...) {
        LOG_GDBSTUB(INFO, "%s", "write memory exception");
        return false;
    }
}

static uint64_t target_read_register(int thread, int reg)
{
    return g_sys_emu->thread_get_reg(to_target_thread(thread), reg);
}

static void target_write_register(int thread, int reg, uint64_t data)
{
    g_sys_emu->thread_set_reg(to_target_thread(thread), reg, data);
}

static bemu::freg_t target_read_fregister(int thread, int reg)
{
    return g_sys_emu->thread_get_freg(to_target_thread(thread), reg);
}

static void target_write_fregister(int thread, int reg, bemu::freg_t data)
{
    g_sys_emu->thread_set_freg(to_target_thread(thread), reg, data);
}

static uint64_t target_read_pc(int thread)
{
    return g_sys_emu->thread_get_pc(to_target_thread(thread));
}

static void target_write_pc(int thread, uint64_t data)
{
    g_sys_emu->thread_set_pc(to_target_thread(thread), data);
}

static uint64_t target_read_csr(int thread, int csr)
{
    try {
        return g_sys_emu->thread_get_csr(to_target_thread(thread), csr);
    }
    catch (...) {
        return 0;
    }
}

static void target_write_csr(int thread, int csr, uint64_t data)
{
    try {
        g_sys_emu->thread_set_csr(to_target_thread(thread), csr, data);
    }
    catch (...) {
    }
}

static void target_step(int thread)
{
    g_sys_emu->thread_set_single_step(to_target_thread(thread));
    g_sys_emu->thread_set_running(to_target_thread(thread));
}

static void target_continue(int thread)
{
    g_sys_emu->thread_set_running(to_target_thread(thread));
}

static void target_run(int thread, uint64_t start_pc, uint64_t end_pc)
{
    LOG_GDBSTUB(DEBUG, "run thread %d from 0x%010lx to 0x%010lx", thread, start_pc, end_pc);
    g_sys_emu->thread_set_single_step(to_target_thread(thread), start_pc, end_pc);
    g_sys_emu->thread_set_running(to_target_thread(thread));
}

static void target_breakpoint_insert(uint64_t addr)
{
    g_sys_emu->breakpoint_insert(addr);
}

static void target_breakpoint_remove(uint64_t addr)
{
    g_sys_emu->breakpoint_remove(addr);
}

static void target_remote_command(char* cmd)
{
    char* tokens[2];
    int   ntokens = strsplit(cmd, " ", tokens, ARRAY_SIZE(tokens));

    LOG_GDBSTUB(DEBUG, "remote command: \"%s\"", cmd);

    if (ntokens == 0)
        return;

    if (strcmp(tokens[0], "log") == 0) {
        if (ntokens > 1) {
            if (strcmp(tokens[1], "on") == 0)
                g_sys_emu->get_logger().setLogLevel(LOG_DEBUG);
            else if (strcmp(tokens[1], "off") == 0)
                g_sys_emu->get_logger().setLogLevel(LOG_INFO);
        }
    }
}

/** RSP protocol ***/

static inline ssize_t rsp_get_char(char* ch)
{
    return recv(g_client_fd, ch, sizeof(*ch), 0);
}

static inline ssize_t rsp_put_char(char ch)
{
    return send(g_client_fd, &ch, sizeof(ch), 0);
}

static inline ssize_t rsp_put_buffer(const void* buf, size_t len)
{
    return send(g_client_fd, buf, len, 0);
}

static inline int rsp_is_token(int ch)
{
    return (ch == RSP_START_TOKEN || ch == RSP_END_TOKEN || ch == RSP_ESCAPE_TOKEN || ch == RSP_RUNLENGTH_TOKEN);
}

static ssize_t rsp_receive_packet(char* packet, unsigned int size)
{
    char         chr;
    char         upper, lower;
    uint8_t      checksum;
    ssize_t      ret = 0;
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
        LOG_GDBSTUB(WARN, "read_packet checksum mismatch, "
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

static ssize_t rsp_send_packet_len(const char* packet, size_t len)
{
    char         buf[GDBSTUB_MAX_PACKET_SIZE];
    size_t       cnt = 0;
    unsigned int sum = 0;

    LOG_GDBSTUB(DEBUG, "send packet: \"%s\"", packet);

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

static inline ssize_t rsp_send_packet(const char* packet)
{
    return rsp_send_packet_len(packet, strlen(packet));
}

static int rsp_is_query_packet(const char* p, const char* query, char separator)
{
    unsigned int query_len = strlen(query);
    return strncmp(p + 1, query, query_len) == 0 && (p[query_len + 1] == '\0' || p[query_len + 1] == separator);
}

/** GDB stub routines ***/

static void gdbstub_handle_qsupported(void)
{
    char reply[GDBSTUB_MAX_PACKET_SIZE + 1];

    LOG_GDBSTUB(DEBUG, "%s", "handle qSupported");

    snprintf(reply, sizeof(reply),
             "PacketSize=%x;qXfer:features:read+;qXfer:threads:read+;vContSupported+;hwbreak+",
             GDBSTUB_MAX_PACKET_SIZE);

    rsp_send_packet(reply);
}

static ssize_t gdbstub_qxfer_send_object(const char* object, size_t object_size,
                                         unsigned long offset, unsigned long length)
{
    char resp;
    char reply[GDBSTUB_MAX_PACKET_SIZE + 2];

    if (offset == object_size) { /* Offset at the end, no more data to be read */
        rsp_send_packet("l");
        return 0;
    }
    if (offset > object_size) { /* Out of bounds */
        rsp_send_packet("E16"); /* EINVAL */
        return -EINVAL;
    }

    if (length > object_size - offset) {
        length = object_size - offset;
        resp   = 'l';
    }
    else {
        resp = 'm'; /* More data to be read */
    }

    if (length > GDBSTUB_MAX_PACKET_SIZE) {
        length = GDBSTUB_MAX_PACKET_SIZE;
    }
    reply[0] = resp;
    strncpy(&reply[1], object + offset, length);
    reply[length + 1] = '\0';
    return rsp_send_packet_len(reply, length + 1);
}

static void gdbstub_handle_qxfer_features_read(const char* annex, const char* offset_length)
{
    const char*   annex_data = NULL;
    size_t        annex_size;
    unsigned long offset = strtoul(offset_length, NULL, 16);
    unsigned long length = strtoul(strchr(offset_length, ',') + 1, NULL, 16);

    LOG_GDBSTUB(DEBUG, "handle qXfer:features:read annex: %s, offset: 0x%lx, size: 0x%lx",
                annex, offset, length);

    for (unsigned int i = 0; i < ARRAY_SIZE(gdbstub_target_descs); i++) {
        if (strcmp(annex, gdbstub_target_descs[i].annex) == 0) {
            annex_data = gdbstub_target_descs[i].data;
            annex_size = gdbstub_target_descs[i].size;
            break;
        }
    }

    if (!annex_data) { /* Annex not found! */
        rsp_send_packet("E00");
        return;
    }

    gdbstub_qxfer_send_object(annex_data, annex_size, offset, length);
}

static void gdbstub_handle_qxfer_features(char* tokens[], int ntokens)
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

static void gdbstub_handle_qxfer_threads(char* tokens[], int ntokens)
{
    unsigned long offset, length;

    /* Generate the list */
    if (g_thread_list_xml == NULL) {
        g_thread_list_xml    = (char*)malloc(1);
        g_thread_list_xml[0] = '\0';

        g_thread_list_xml = strconcat(g_thread_list_xml,
                                      "<?xml version=\"1.0\"?>\n"
                                      "<!DOCTYPE threads SYSTEM \"threads.dtd\">\n"
                                      "<threads>\n");

        for (unsigned shire = 0; shire < EMU_NUM_SHIRES; shire++) {
            unsigned minion_count = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_MINIONS_PER_SHIRE);
            unsigned thread_count = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_MINION);
            unsigned shire_id     = (shire == EMU_IO_SHIRE_SP ? IO_SHIRE_ID : shire);

            for (unsigned minion = 0; minion < minion_count; minion++) {
                unsigned minion_id = minion + EMU_MINIONS_PER_SHIRE * shire_id;

                for (unsigned thread = 0; thread < thread_count; thread++) {
                    unsigned thread_id     = thread + minion_id * EMU_THREADS_PER_MINION;
                    bool     is_sp         = thread_id == IO_SHIRE_SP_HARTID;
                    unsigned gdb_thread_id = to_gdb_thread(thread_id);

                    if (target_thread_exists(gdb_thread_id)) {
                        char desc[512];
                        snprintf(desc, sizeof(desc),
                                 "    <thread id=\"%X\" core=\"%d\" name=\"S%d:M%d:T%d%s\"></thread>\n",
                                 gdb_thread_id, minion_id, shire_id, minion, thread, is_sp ? " (SP)" : "");
                        g_thread_list_xml = strconcat(g_thread_list_xml, desc);
                    }
                }
            }
        }

        g_thread_list_xml = strconcat(g_thread_list_xml, "</threads>");
    }

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
    length = strtoul(strchr(tokens[3], ',') + 1, NULL, 16);

    gdbstub_qxfer_send_object(g_thread_list_xml, strlen(g_thread_list_xml), offset, length);
}

static void gdbstub_handle_qxfer(char* packet)
{
    char* tokens[5];
    int   ntokens = strsplit(packet, ":", tokens, ARRAY_SIZE(tokens));

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
    LOG_GDBSTUB(DEBUG, "%s", "handle qfThreadInfo");

    /* qXfer:threads has preference over this method. Reply: not supported. */
    rsp_send_packet("");
}

static void gdbstub_handle_qsthreadinfo(void)
{
    LOG_GDBSTUB(DEBUG, "%s", "handle qsThreadInfo");

    /* End of list */
    rsp_send_packet("l");
}

static void gdbstub_handle_qgettlsaddr(char* packet)
{
    LOG_GDBSTUB(DEBUG, "%s", "handle qGetTLSAddr");

    char* tokens[2];
    char* tokens2[3];
    char tmp[16+1];

    tmp[0] = '\0';
    int ntokens = strsplit(packet, ":", tokens, ARRAY_SIZE(tokens));
    int ntokens2 = strsplit(tokens[1], ",", tokens2, ARRAY_SIZE(tokens2));

    if ((ntokens < 2) || (ntokens2 < 3)) {
        rsp_send_packet("");
        return;
    }

    //lm parameter is not taken in consideration

    int thread_id = strtol(tokens2[0], NULL, 16);
    uint64_t tp = target_read_register(thread_id, TP_REG);

    //point to local thread variable under consideration
    int offset_thread_var = strtol(tokens2[1], NULL, 16);
    u64_to_hexstr_raw(tmp, (tp + offset_thread_var));

    rsp_send_packet(tmp);
}


static void gdbstub_handle_read_general_registers(void)
{
    char reply[((NUM_XREGS + 1) * 8 + NUM_FREGS * 32 + NUM_CSR_FREGS * 4) * 2 + 1];
    char tmp[(bemu::VLEN / 8) * 2 + 1];
    reply[0] = '\0';

    /* General purpose registers */
    for (int i = 0; i < NUM_XREGS; i++) {
        u64_to_hexstr(tmp, target_read_register(g_cur_general_thread, i));
        strcat(reply, tmp);
    }

    /* PC */
    u64_to_hexstr(tmp, target_read_pc(g_cur_general_thread));
    strcat(reply, tmp);

    /* Floating-point vector registers */
    for (int i = 0; i < NUM_FREGS; i++) {
        freg_to_hexstr(tmp, target_read_fregister(g_cur_general_thread, i));
        strcat(reply, tmp);
    }

    /* Floating-point related CSRs */
    u32_to_hexstr(tmp, (uint32_t)target_read_csr(g_cur_general_thread, bemu::CSR_FFLAGS));
    strcat(reply, tmp);
    u32_to_hexstr(tmp, (uint32_t)target_read_csr(g_cur_general_thread, bemu::CSR_FRM));
    strcat(reply, tmp);
    u32_to_hexstr(tmp, (uint32_t)target_read_csr(g_cur_general_thread, bemu::CSR_FCSR));
    strcat(reply, tmp);

    rsp_send_packet(reply);
}

static void gdbstub_handle_read_register(const char* packet)
{
    char     buffer[(bemu::VLEN / 8) * 2 + 1];
    uint64_t reg = strtoul(packet + 1, NULL, 16);

    LOG_GDBSTUB(DEBUG, "read register: %" PRIu64, reg);

    if (/* reg >= XREGS_START && */ reg <= XREGS_END) {
        u64_to_hexstr(buffer, target_read_register(g_cur_general_thread, reg));
    }
    else if (reg == PC_REG) { /* PC register */
        u64_to_hexstr(buffer, target_read_pc(g_cur_general_thread));
    }
    else if (reg >= FREGS_START && reg <= FREGS_END) { /* Floating-point vector registers */
        freg_to_hexstr(buffer, target_read_fregister(g_cur_general_thread, reg - FREGS_START));
    }
    else if (reg >= CSR_FREGS_START && reg <= CSR_FREGS_END) { /* fflags, frm, fcsr */
        int fcsr = bemu::CSR_FFLAGS + (reg - CSR_FREGS_START);
        u32_to_hexstr(buffer, target_read_csr(g_cur_general_thread, fcsr));
    }
    else if (reg >= CSR_REGS_START && reg <= CSR_REGS_END) {
        int csr = target_csr_list[reg - CSR_REGS_START];
        u64_to_hexstr(buffer, target_read_csr(g_cur_general_thread, csr));
    }
    else {
        LOG_GDBSTUB(INFO, "read register: unknown register %" PRIu64, reg);
        rsp_send_packet("E00");
        return;
    }

    rsp_send_packet(buffer);
}

static void gdbstub_handle_write_register(const char* packet)
{
    uint64_t    reg    = strtoul(packet + 1, NULL, 16);
    const char* valuep = strchr(packet + 1, '=') + 1;

    LOG_GDBSTUB(DEBUG, "write register: %" PRIu64 " <- \"%s\"", reg, valuep);

    if (/* reg >= XREGS_START && */ reg <= XREGS_END) {
        target_write_register(g_cur_general_thread, reg, hexstr_to_u64(valuep));
    }
    else if (reg == PC_REG) { /* PC register */
        target_write_pc(g_cur_general_thread, hexstr_to_u64(valuep));
    }
    else if (reg >= FREGS_START && reg <= FREGS_END) { /* Floating-point vector registers */
        target_write_fregister(g_cur_general_thread, reg - FREGS_START, hexstr_to_freg(valuep));
    }
    else if (reg >= CSR_FREGS_START && reg <= CSR_FREGS_END) { /* fflags, frm, fcsr */
        int fcsr = bemu::CSR_FFLAGS + (reg - CSR_FREGS_START);
        target_write_csr(g_cur_general_thread, fcsr, hexstr_to_u32(valuep));
    }
    else if (reg >= CSR_REGS_START && reg <= CSR_REGS_END) {
        int csr = target_csr_list[reg - CSR_REGS_START];
        target_write_csr(g_cur_general_thread, csr, hexstr_to_u32(valuep));
    }
    else {
        LOG_GDBSTUB(INFO, "write register: unknown register %" PRIu64, reg);
        rsp_send_packet("E00");
        return;
    }

    rsp_send_packet("OK");
}

static void gdbstub_handle_set_thread(const char* packet)
{
    char op = packet[1];

    LOG_GDBSTUB(DEBUG, "handle set_thread, op: %c", op);

    if (op == 'g') {
        g_cur_general_thread = strtol(&packet[2], NULL, 16);
        /* 0 means pick any thread */
        if (g_cur_general_thread == 0)
            g_cur_general_thread = 1;
        LOG_GDBSTUB(DEBUG, "setting general thread to: %d", g_cur_general_thread);
    }
    else if (op == 'c') {
        /* We support vCont, this is deprecated */
        rsp_send_packet("");
        return;
    }
    else {
        LOG_GDBSTUB(INFO, "handle set_thread: invalid op: %c", op);
        return;
    }

    rsp_send_packet("OK");
}

static void gdbstub_handle_query_thread(void)
{
    /* We support vCont, this is deprecated */
    rsp_send_packet("");
}

static void gdbstub_handle_qrcmd(const char* packet)
{
    char        cmd[GDBSTUB_MAX_PACKET_SIZE + 1];
    const char* cmd_hex = strchr(packet, ',');

    if (!cmd_hex) {
        rsp_send_packet("");
        return;
    }

    hextostr(cmd, cmd_hex + 1);
    target_remote_command(cmd);
    rsp_send_packet("OK");
}

static void gdbstub_handle_thread_alive(const char* packet)
{
    int thread_id;

    LOG_GDBSTUB(DEBUG, "%s", "handle thread alive");

    thread_id = strtol(&packet[1], NULL, 16);

    if (target_thread_exists(thread_id) && target_thread_is_alive(thread_id))
        rsp_send_packet("OK");
    else
        rsp_send_packet("E00");
}

static inline void gdbstub_handle_vcont_action(char* action, int thread)
{
    if (target_thread_exists(thread)) {
        switch (action[0]) {
        case 's':
        case 'S':
            target_step(thread);
            break;
        case 'c':
        case 'C':
            target_continue(thread);
            break;
        case 'r': {
            uint64_t start_pc = strtoul(action + 1, &action, 16);
            uint64_t end_pc   = strtoul(action + 1, NULL, 16);
            target_run(thread, start_pc, end_pc);
            break;
        }
        default:
            break;
        }
    }
}

static void gdbstub_handle_vcont(char* packet)
{
    char* action;

    LOG_GDBSTUB(DEBUG, "%s", "handle vCont");

    /* For each action */
    action = strtok(packet + 5, ";");
    while (action != NULL) {
        int thread = THREAD_ID_ALL_THREADS;
        /* Find optional thread-id */
        char* threadptr = strchr(action, ':');
        if (threadptr) {
            thread     = strtol(threadptr + 1, NULL, 16);
            *threadptr = '\0';
        }

        LOG_GDBSTUB(DEBUG, "vCont action %s, thread: %d", action, thread);

        if (thread == THREAD_ID_ALL_THREADS) {
            for (unsigned id = 1; id <= target_num_threads(); id++) {
                gdbstub_handle_vcont_action(action, id);
            }
        }
        else {
            gdbstub_handle_vcont_action(action, thread);
        }

        action = strtok(NULL, ";");
    }

    /* The response is sent when something (such a breakpoint) happens */
}

static bool parse_breakpoint(char* packet, char* type, uint64_t* addr, uint64_t* kind)
{
    char* tokens[3];
    int   ntokens = strsplit(packet, ",", tokens, ARRAY_SIZE(tokens));

    if (ntokens < 3)
        return false;

    *type = tokens[0][1];
    *addr = strtoull(tokens[1], NULL, 16);
    *kind = strtoull(tokens[2], NULL, 16);
    return true;
}

static void gdbstub_handle_breakpoint_insert(char* packet)
{
    char     type;
    uint64_t addr, kind;

    if (!parse_breakpoint(packet, &type, &addr, &kind)) {
        LOG_GDBSTUB(INFO, "insert breakpoint: %s", "unknown parameters");
        rsp_send_packet("");
        return;
    }

    LOG_GDBSTUB(DEBUG, "insert breakpoint %c 0x%" PRIx64 " %" PRIu64, type, addr, kind);

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

static void gdbstub_handle_breakpoint_remove(char* packet)
{
    char     type;
    uint64_t addr, kind;

    if (!parse_breakpoint(packet, &type, &addr, &kind)) {
        LOG_GDBSTUB(INFO, "remove breakpoint: %s", "unknown parameters");
        rsp_send_packet("");
        return;
    }

    LOG_GDBSTUB(DEBUG, "remove breakpoint %c 0x%" PRIx64 " %" PRIu64, type, addr, kind);

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

static void gdbstub_handle_read_memory(const char* packet)
{
    char*    p;
    uint64_t addr, length;
    uint8_t  data[GDBSTUB_MAX_PACKET_SIZE / 2];
    char     send[2 * sizeof(data) + 1];

    addr = strtoull(packet + 1, &p, 16);
    if (*p == ',')
        p++;
    length = strtoull(p, NULL, 16);

    LOG_GDBSTUB(DEBUG, "read memory: from 0x%" PRIx64 ", size 0x%" PRIx64, addr, length);

    if (!target_read_memory(g_cur_general_thread, addr, data, length))
        rsp_send_packet("E01");

    memtohex(send, data, length);

    rsp_send_packet(send);
}

static void gdbstub_handle_write_memory(const char* packet)
{
    char*    p;
    uint64_t addr, length;
    uint8_t  data[GDBSTUB_MAX_PACKET_SIZE / 2];

    addr = strtoull(packet + 1, &p, 16);
    if (*p == ',')
        p++;
    length = strtoull(p, &p, 16);

    LOG_GDBSTUB(DEBUG, "write memory: from 0x%" PRIx64 ", size 0x%" PRIx64, addr, length);

    hextomem(data, p, length);

    if (!target_write_memory(g_cur_general_thread, addr, data, length))
        rsp_send_packet("E01");

    rsp_send_packet("OK");
}

static int gdbstub_handle_packet(char* packet)
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
        gdbstub_fini();
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
        else if (rsp_is_query_packet(packet, "GetTLSAddr", ':'))
            gdbstub_handle_qgettlsaddr(packet);
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
        }
        else {
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
        LOG_GDBSTUB(DEBUG, "unrecognized command \"%c\"", packet[0]);
        rsp_send_packet("");
        break;
    }

    return 0;
}

static int gdbstub_open_port(unsigned short* port)
{
    struct sockaddr_in sockaddr;
    int                fd, ret, opt;
    socklen_t          len = sizeof(sockaddr);

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_GDBSTUB(INFO, "socket error: %d", fd);
        return fd;
    }

    /* Allow rapid reuse of this port */
    opt = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
        LOG_GDBSTUB(INFO, "setsockopt(SO_REUSEADDR) error: %d", ret);
        close(fd);
        return ret;
    }

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(GDBSTUB_DEFAULT_PORT);
    sockaddr.sin_addr.s_addr = 0;
    ret                      = bind(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if (ret < 0) {
        sockaddr.sin_family      = AF_INET;
        sockaddr.sin_port        = htons(0); /* Automatic port */
        sockaddr.sin_addr.s_addr = 0;
        ret                      = bind(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
        if (ret < 0) {
            LOG_GDBSTUB(INFO, "bind error: %d", ret);
            close(fd);
            return ret;
        }
    }

    ret = getsockname(fd, (struct sockaddr*)&sockaddr, &len);
    if (ret < 0) {
        LOG_GDBSTUB(INFO, "getsockname error: %d", ret);
        close(fd);
        return ret;
    }

    *port = ntohs(sockaddr.sin_port);

    ret = listen(fd, 1);
    if (ret < 0) {
        LOG_GDBSTUB(INFO, "listen error: %d", ret);
        close(fd);
        return -1;
    }

    return fd;
}

static int gdbstub_accept(int listen_fd)
{
    struct sockaddr_in sockaddr;
    socklen_t          len;
    int                fd, ret, opt;

    len = sizeof(sockaddr);
    fd  = accept(listen_fd, (struct sockaddr*)&sockaddr, &len);
    if (fd < 0) {
        LOG_GDBSTUB(INFO, "accept error: %d", fd);
        return fd;
    }

    /* Disable Nagle - allow small packets to be sent without delay. */
    opt = 1;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    if (ret < 0) {
        LOG_GDBSTUB(INFO, "setsockopt(TCP_NODELAY) error: %d", ret);
        close(fd);
        return ret;
    }

    LOG_GDBSTUB(INFO, "Client %s connected", inet_ntoa(sockaddr.sin_addr));

    return fd;
}

int gdbstub_init(sys_emu* emu, bemu::System* chip)
{
    unsigned short port;

    if (g_status != GDBSTUB_STATUS_NOT_INITIALIZED)
        return -1;

    g_sys_emu = emu;
    g_agent   = bemu::Noagent(chip, "GDB-stub");

    LOG_GDBSTUB(INFO, "%s", "Initializing...");

    g_listen_fd = gdbstub_open_port(&port);
    if (g_listen_fd < 0) {
        return g_listen_fd;
    }

    LOG_GDBSTUB(INFO, "Listening on port %d ...", port);

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

    LOG_GDBSTUB(INFO, "%s", "Client connection closed");

    g_status = GDBSTUB_STATUS_WAITING_CLIENT;
    g_sys_emu->disconnect_gdbstub();
    return 0;
}

void gdbstub_fini()
{
    LOG_GDBSTUB(INFO, "%s", "Finishing...");

    if (g_listen_fd > 0) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }

    (void)gdbstub_close_client();

    if (g_thread_list_xml) {
        free(g_thread_list_xml);
        g_thread_list_xml = NULL;
    }

    g_status  = GDBSTUB_STATUS_NOT_INITIALIZED;
    g_sys_emu = nullptr;
}

int gdbstub_io()
{
    ssize_t       ret;
    struct pollfd pollfd;
    char          packet[GDBSTUB_MAX_PACKET_SIZE + 1];

    if (g_status != GDBSTUB_STATUS_RUNNING)
        return -1;

    memset(&pollfd, 0, sizeof(pollfd));
    pollfd.fd     = g_client_fd;
    pollfd.events = POLLIN;

    /* Return immediately */
    ret = poll(&pollfd, 1, 0);
    if (ret <= 0)
        return ret;

    ret = rsp_receive_packet(packet, sizeof(packet) - 1);
    if (ret < 0) {
        LOG_GDBSTUB(WARN, "RSP: error receiving packet: %s",
                    strerror(ret));
        gdbstub_fini();
        return ret;
    }
    else if (ret == 0) {
        return 0;
    }

    LOG_GDBSTUB(DEBUG, "recv packet: \"%s\"", packet);

    return gdbstub_handle_packet(packet);
}

void gdbstub_signal_break(int thread)
{
    const int signal = 5;
    char      buffer[32];
    int       len;
    int       gdb_thread = to_gdb_thread(thread);

    len = snprintf(buffer, sizeof(buffer), "T%02Xthread:%02X;", signal, gdb_thread);
    rsp_send_packet_len(buffer, len);

    /*
     * "Whenever GDB stops your program, due to a breakpoint or a signal,
     * it automatically selects the thread where that breakpoint or signal happened."
     */
    g_cur_general_thread = gdb_thread;
}

enum gdbstub_status gdbstub_get_status()
{
    return g_status;
}
