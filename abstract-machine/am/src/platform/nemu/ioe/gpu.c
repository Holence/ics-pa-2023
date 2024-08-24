#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t wh = inl(VGACTL_ADDR);
  *cfg = (AM_GPU_CONFIG_T){
      .present = true, .has_accel = false, .width = wh >> 16, .height = wh & 0xFFFF, .vmemsz = 0};
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *fb = (uint32_t *)FB_ADDR;
  int fb_w = inl(VGACTL_ADDR) >> 16;
  uint32_t *pixels = (uint32_t *)ctl->pixels;
  int index = 0;
  for (int x = ctl->x; x < ctl->x + ctl->w; x++) {
    for (int y = ctl->y; y < ctl->y + ctl->h; y++) {
      fb[y * fb_w + x] = pixels[index++];
    }
  }
  outl(SYNC_ADDR, ctl->sync);
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
