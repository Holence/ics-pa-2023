#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
#define MULTIPROGRAM_YIELD() yield()
#else
#define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
    [AM_KEY_NONE] = "NONE",
    AM_KEYS(NAME)};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  // 由于串口是一个字符设备, 对应的字节序列没有"位置"的概念, 因此serial_write()中的offset参数可以忽略
  size_t ret = 0;
  char *ptr = (char *)buf;
  for (size_t i = 0; i < len; i++) {
    putch(ptr[i]);
    ret++;
  }
  return ret;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
