#include <am.h>
#include <nemu.h>
#include <string.h>
#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T){
      .present = true, .has_accel = false, .width = inw(VGACTL_ADDR + 2), .height = inw(VGACTL_ADDR), .vmemsz = 0};
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *fb = (uint32_t *)FB_ADDR;
  int fb_w = inw(VGACTL_ADDR + 2);
  uint32_t *pixels = (uint32_t *)ctl->pixels;
  int index = 0;
  for (int y = ctl->y; y < ctl->y + ctl->h; y++) {
    // for (int x = ctl->x; x < ctl->x + ctl->w; x++) {
    //   fb[y * fb_w + x] = pixels[index++];
    // }
    memcpy(fb + y * fb_w + ctl->x, pixels + index, ctl->w << 2);
    index += ctl->w;
  }
  outl(SYNC_ADDR, ctl->sync);
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
