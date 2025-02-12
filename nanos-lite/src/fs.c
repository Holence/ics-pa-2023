#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);
size_t sb_write(const void *buf, size_t offset, size_t len);
size_t sbctl_write(const void *buf, size_t offset, size_t len);
size_t sbctl_read(void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn)(void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn)(const void *buf, size_t offset, size_t len);

typedef struct {
  char *name; // 只有单层目录 文件名就是一整个fsimg下的整个path /bin/hello
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;   // 约定为NULL时表示普通文件
  WriteFn write; // 约定为NULL时表示普通文件
} Finfo;

enum { FD_STDIN,
       FD_STDOUT,
       FD_STDERR,

       // keyboard event
       FD_EVENT,

       // frame buffer 一个W * H * 4字节的数组
       // 按行优先存储所有像素的颜色值(32位)
       // 每个像素是`00rrggbb`的形式, 8位颜色
       FD_FB,

       FD_SB,
       FD_SBCTL,
};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("invalid_read: should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("invalid_write: should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
    [FD_STDIN] = {"stdin", 0, 0, 0, invalid_read, invalid_write},
    [FD_STDOUT] = {"stdout", 0, 0, 0, invalid_read, serial_write},
    [FD_STDERR] = {"stderr", 0, 0, 0, invalid_read, serial_write},
    [FD_EVENT] = {"/dev/events", 0, 0, 0, events_read, invalid_write},
    [FD_FB] = {"/dev/fb", 0, 0, 0, invalid_read, fb_write},
    [FD_SB] = {"/dev/sb", 0, 0, 0, invalid_read, sb_write},
    [FD_SBCTL] = {"/dev/sbctl", 0, 0, 0, sbctl_read, sbctl_write},
    {"/proc/dispinfo", 0, 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};

inline char *get_file_name(int fd) {
  return file_table[fd].name;
}

inline size_t get_disk_offset(int fd) {
  return file_table[fd].disk_offset;
}

bool file_exist(const char *pathname) {
  for (int i = 0; i < LENGTH(file_table); i++) {
    if (strcmp(file_table[i].name, pathname) == 0) {
      return true;
    }
  }
  return false;
}

int fs_open(const char *pathname, int flags, int mode) {
  // 为了简化实现, 我们允许所有用户程序都可以对所有已存在的文件进行读写
  // 所以可以忽略flags和mode了
  int fd = -1;
  for (int i = 0; i < LENGTH(file_table); i++) {
    if (strcmp(file_table[i].name, pathname) == 0) {
      fd = i;
      break;
    }
  }
  if (fd == -1) {
    Warning("Cannot find file: %s", pathname);
    return -1;
  } else {
    file_table[fd].open_offset = 0; // 每次打开文件，重置指针
    return fd;
  }
}

size_t fs_read(int fd, void *buf, size_t len) {
  if (file_table[fd].read) {
    size_t ret = file_table[fd].read(buf, file_table[fd].open_offset, len);
    if (file_table[fd].size > 0) {
      file_table[fd].open_offset += len;
    }
    return ret;
  } else {
    // 若偏移量超过边界，则读到文件结尾
    if (file_table[fd].open_offset + len > file_table[fd].size) {
      len = file_table[fd].size - file_table[fd].open_offset;
    }
    size_t ret = ramdisk_read(buf, file_table[fd].open_offset + file_table[fd].disk_offset, len);
    file_table[fd].open_offset += len;
    return ret;
  }
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (file_table[fd].write) {
    size_t ret = file_table[fd].write(buf, file_table[fd].open_offset, len);
    if (file_table[fd].size > 0) {
      file_table[fd].open_offset += len;
    }
    return ret;
  } else {
    // 若偏移量超过边界，则写到文件结尾
    if (file_table[fd].open_offset + len > file_table[fd].size) {
      len = file_table[fd].size - file_table[fd].open_offset;
    }
    size_t ret = ramdisk_write(buf, file_table[fd].open_offset + file_table[fd].disk_offset, len);
    file_table[fd].open_offset += len;
    return ret;
  }
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  size_t new_offset;
  switch (whence) {
  case SEEK_SET:
    new_offset = offset;
    break;
  case SEEK_CUR:
    new_offset = file_table[fd].open_offset + offset;
    break;
  case SEEK_END:
    new_offset = file_table[fd].size + offset;
    break;
  default:
    return -1;
  }
  file_table[fd].open_offset = new_offset;
  return new_offset;
}

int fs_close(int fd) {
  // 由于sfs没有维护文件打开的状态, fs_close()可以直接返回0, 表示总是关闭成功.
  return 0;
}

void init_fs() {
  // initialize the size of /dev/fb
  AM_GPU_CONFIG_T info = io_read(AM_GPU_CONFIG);
  file_table[FD_FB].size = (info.width * info.height) << 2;
}
