#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn)(void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn)(const void *buf, size_t offset, size_t len);

typedef struct {
  char *name; // 只有单层目录 文件名就是一整个fsimg下的整个path /bin/hello
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum { FD_STDIN,
       FD_STDOUT,
       FD_STDERR,
       FD_FB };

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
    [FD_STDIN] = {"stdin", 0, 0, 0, invalid_read, invalid_write},
    [FD_STDOUT] = {"stdout", 0, 0, 0, invalid_read, invalid_write},
    [FD_STDERR] = {"stderr", 0, 0, 0, invalid_read, invalid_write},
#include "files.h"
};

inline char *get_file_name(int fd) {
  return file_table[fd].name;
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
    panic("Cannot find file: %s", pathname);
  } else {
    return fd;
  }
}

size_t fs_read(int fd, void *buf, size_t len) {
  if (fd > FD_STDERR) {
    // 若偏移量超过边界，则读完
    if (file_table[fd].open_offset + len > file_table[fd].size) {
      len = file_table[fd].size - file_table[fd].open_offset;
    }
    size_t ret = ramdisk_read(buf, file_table[fd].open_offset + file_table[fd].disk_offset, len);
    file_table[fd].open_offset += len;
    return ret;
  } else {
    return 0;
  }
}

size_t fs_write(int fd, const void *buf, size_t len) {
  int ret = 0;
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    char *ptr = (char *)buf;
    for (size_t i = 0; i < len; i++) {
      putch(ptr[i]);
      ret++;
    }
    return ret;
  } else if (fd > FD_STDERR) {
    // 注意偏移量不要越过文件的边界
    assert(file_table[fd].open_offset + len <= file_table[fd].size);
    size_t ret = ramdisk_write(buf, file_table[fd].open_offset + file_table[fd].disk_offset, len);
    file_table[fd].open_offset += len;
    return ret;
  }
  return -1;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd > FD_STDERR) {
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
    // 注意偏移量不要越过文件的边界
    assert(new_offset >= 0 && new_offset <= file_table[fd].size);
    file_table[fd].open_offset = new_offset;
    return new_offset;
  } else {
    return -1;
  }
}

int fs_close(int fd) {
  // 由于sfs没有维护文件打开的状态, fs_close()可以直接返回0, 表示总是关闭成功.
  return 0;
}

void init_fs() {
  // TODO: initialize the size of /dev/fb
}
