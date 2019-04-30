#ifndef ESPRT_IFACE_H
#define ESPRT_IFACE_H

#define ESPRT_DEVICE_FILENAME "/dev/esprt"

#define ESPRT_CHR_IOCTL_MAGIC 0x67879
struct esprt_ioctl_cmd_define_t {
  uint64_t addr;
  uint64_t size;
  int is_exec;
};

struct esprt_ioctl_cmd_read_t {
  uint64_t addr;
  uint64_t size;
  void *user_buf;
};

struct esprt_ioctl_cmd_write_t {
  uint64_t addr;
  uint64_t size;
  void *user_buf;
};

struct esprt_ioctl_cmd_launch_t {
  uint64_t launch_pc;
};

struct esprt_ioctl_cmd_boot_t {
  uint64_t init_pc;
  uint64_t trap_pc;
};

/* actual numbers TBD!!! */
#define ESPRT_IOCTL_CMD_DEFINE                                                 \
  _IOW(ESPRT_CHR_IOCTL_MAGIC, 1, struct esprt_ioctl_cmd_define_t)
#define ESPRT_IOCTL_CMD_READ                                                   \
  _IOW(ESPRT_CHR_IOCTL_MAGIC, 2, struct esprt_ioctl_cmd_read_t)
#define ESPRT_IOCTL_CMD_WRITE                                                  \
  _IOW(ESPRT_CHR_IOCTL_MAGIC, 3, struct esprt_ioctl_cmd_write_t)
#define ESPRT_IOCTL_CMD_LAUNCH                                                 \
  _IOW(ESPRT_CHR_IOCTL_MAGIC, 4, struct esprt_ioctl_cmd_launch_t)
#define ESPRT_IOCTL_CMD_BOOT                                                   \
  _IOW(ESPRT_CHR_IOCTL_MAGIC, 5, struct esprt_ioctl_cmd_boot_t)

#endif // ESPRT_IFACE_H
