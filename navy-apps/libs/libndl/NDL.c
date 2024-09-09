#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_x = 0, canvas_y = 0;
static int canvas_w = 0, canvas_h = 0;

// 以毫秒(10^-3秒)为单位返回系统时间
uint32_t NDL_GetTicks() {
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000 + t.tv_usec / 1000;
}

// 读出一条事件信息, 将其写入`buf`中, 最长写入`len`字节
// 若读出了有效的事件, 函数返回1, 否则返回0
int NDL_PollEvent(char *buf, int len) {
  int fd = open("/dev/events", O_RDONLY);
  int ret = read(fd, buf, len);

  // 怎样判断有效的事件？
  if (ret > 0) {
    return 1;
  } else {
    return 0;
  }
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w;
    screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0)
        continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0)
        break;
    }
    close(fbctl);
  }

  // 打开一张(*w) X (*h)的画布
  // 如果*w和*h均为0, 则将系统全屏幕作为画布, 并将*w和*h分别设为系统屏幕的大小
  char buf[32];
  int fd = open("/proc/dispinfo", O_RDONLY);
  read(fd, buf, 32);
  sscanf(buf, "WIDTH:%d\nHEIGHT:%d", &screen_w, &screen_h);
  if (*w == 0 && *h == 0) {
    *w = screen_w;
    *h = screen_h;
  } else {
    if (*w > screen_w) {
      *w = screen_w;
    }
    if (*h > screen_h) {
      *h = screen_h;
    }
  }
  canvas_w = *w;
  canvas_h = *h;
  canvas_x = (screen_w - canvas_w) / 2;
  canvas_y = (screen_h - canvas_h) / 2;
}

// 向画布`(x, y)`坐标处绘制`w*h`的矩形图像, 并将该绘制区域同步到屏幕上
// 图像像素按行优先方式存储在`pixels`中, 每个像素用32位整数以`00RRGGBB`的方式描述颜色
// pixels是长度为 w*h 的数组
void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  int rect_width_bytes = w << 2;
  int screen_width_bytes = screen_w << 2;
  int seek_amount = screen_width_bytes - rect_width_bytes;
  int fd = open("/dev/fb", O_WRONLY);
  lseek(fd, screen_width_bytes * canvas_y + (canvas_x << 2), SEEK_SET); // 跳到[canvas_x, canvas_y]的地方
  lseek(fd, screen_width_bytes * y + (x << 2), SEEK_CUR);               // 跳到[canvas_x + x, canvas_y + y]的地方
  for (int i = 0; i < h; i++) {
    write(fd, pixels, rect_width_bytes); // 写一整行，其中的open_offset+=rect_width_bytes的
    lseek(fd, seek_amount, SEEK_CUR);    // 跳到下一行的位置（这里最后一次循环的时候，可能会超出fb.size的边界，但并不用error）
    pixels += w;                         // w不用乘4,因为它已经是(uint32_t *)了
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  return 0;
}

void NDL_Quit() {
}
