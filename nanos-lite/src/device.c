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
  yield(); // PA4 设备访问缓慢，切换进程
  size_t ret = 0;
  char *ptr = (char *)buf;
  for (size_t i = 0; i < len; i++) {
    putch(ptr[i]);
    ret++;
  }
  return ret;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  // 读出一个键盘事件
  // - 按下按键事件, 如kd RETURN表示按下回车键
  // - 松开按键事件, 如ku A表示松开A键
  yield(); // PA4 设备访问缓慢，切换进程
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE) {
    return 0;
  } else {
    return snprintf(buf, len, "k%c %s\n", ev.keydown ? 'd' : 'u', keyname[ev.keycode]);
  }
}

// /proc/dispinfo的格式：
// WIDTH:640
// HEIGHT:480
static int screen_w, screen_h;
size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T info = io_read(AM_GPU_CONFIG);
  screen_w = info.width;
  screen_h = info.height;
  return snprintf(buf, len, "WIDTH:%d\nHEIGHT:%d", screen_w, screen_h);
}

// 向frame buffer中写入一行，buf为需要写入的一行数据，len为字节数，所以一行的宽度为len/4，写完之后刷新屏幕
// offset = (vga_y * screen_w + vga_x) * 4
size_t fb_write(void *buf, size_t offset, size_t len) {
  yield(); // PA4 设备访问缓慢，切换进程
  offset = offset >> 2;
  int vga_x = offset % screen_w;
  int vga_y = offset / screen_w;
  io_write(AM_GPU_FBDRAW, vga_x, vga_y, buf, len >> 2, 1, true);
  return len;
}

// 如果声卡的流缓冲区空闲位置不足, 写操作将会等待, 直到音频数据完全写入流缓冲区之后才会返回
// am中的audio已经实现了阻塞，这里直接调用就行了
size_t sb_write(void *buf, size_t offset, size_t len) {
  Area sbuf;
  sbuf.start = buf;
  sbuf.end = buf + len;
  io_write(AM_AUDIO_PLAY, sbuf);
  return len;
}

// 初始化声卡设备, 应用程序需要一次写入3个int整数共12字节, 3个整数会被依次解释成freq, channels, samples
size_t sbctl_write(void *buf, size_t offset, size_t len) {
  assert(len == sizeof(int) * 3);
  int *ptr = (int *)buf;
  io_write(AM_AUDIO_CTRL, ptr[0], ptr[1], ptr[2]);
  return sizeof(int) * 3;
}

// 查询声卡设备的状态, 应用程序可以读出一个int整数, 表示当前声卡设备流缓冲区的空闲字节数
size_t sbctl_read(void *buf, size_t offset, size_t len) {
  assert(len == sizeof(int));
  int left = io_read(AM_AUDIO_CONFIG).bufsize - io_read(AM_AUDIO_STATUS).count;
  *((int *)buf) = left;
  return sizeof(int);
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
