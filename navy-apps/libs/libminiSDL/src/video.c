#include <NDL.h>
#include <sdl-video.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

// 把src的srcrect区域中的pixels 复制到 dst的dstrect区域中pixels
void SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  // printf("SDL_BlitSurface(%x, %x, %x, %x)\n", src, srcrect, dst, dstrect);
  assert(dst && src);
  assert(dst->format->BitsPerPixel == 32 || dst->format->BitsPerPixel == 8);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);

  // 全屏刷新
  if (srcrect == NULL && dstrect == NULL) {
    if (dst->format->BitsPerPixel == 32) {
      memcpy(dst->pixels, src->pixels, (src->w * src->h) << 2);
    } else {
      memcpy(dst->pixels, src->pixels, src->w * src->h);
    }
  }

  // rect刷新
  else {
    int rect_w, rect_h, src_x, src_y, dst_x, dst_y;
    if (srcrect) {
      rect_w = srcrect->w;
      rect_h = srcrect->h;
      src_x = srcrect->x;
      src_y = srcrect->y;
    } else {
      // If srcrect == NULL, the entire surface is copied.
      rect_w = src->w;
      rect_h = src->h;
      src_x = 0;
      src_y = 0;
    }

    if (dstrect) {
      dst_x = dstrect->x;
      dst_y = dstrect->y;
    } else {
      // If dstrect == NULL, then the destination position (upper left corner) is (0, 0).
      dst_x = 0;
      dst_y = 0;
    }

#define DO                                 \
  {                                        \
    int dst_seek_amount = dst->w - rect_w; \
    int src_seek_amount = src->w - rect_w; \
    dst_pixels += dst_y * dst->w + dst_x;  \
    src_pixels += src_y * src->w + src_x;  \
    for (int i = 0; i < rect_h; ++i) {     \
      for (int j = 0; j < rect_w; ++j) {   \
        *(dst_pixels++) = *(src_pixels++); \
      }                                    \
      dst_pixels += dst_seek_amount;       \
      src_pixels += src_seek_amount;       \
    }                                      \
  }
    if (dst->format->BitsPerPixel == 32) {
      uint32_t *src_pixels = (uint32_t *)src->pixels;
      uint32_t *dst_pixels = (uint32_t *)dst->pixels;
      DO;
    } else {
      uint8_t *src_pixels = (uint8_t *)src->pixels;
      uint8_t *dst_pixels = (uint8_t *)dst->pixels;
      DO;
    }
#undef DO
  }
}

// 把color填入dst的dstrect区域的pixels中
// This function performs a fast fill of the given rectangle with color. If dstrect is NULL, the whole surface will be filled with color.
void SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color) {
  // printf("SDL_FillRect(%x, %x, %x)\n", dst, dstrect, color);
  assert(dst->format->BitsPerPixel == 32 || dst->format->BitsPerPixel == 8);
  CallbackHelper();

#define DO                                       \
  {                                              \
    int width = dst->w;                          \
    int rect_w, rect_h;                          \
    if (dstrect == NULL) {                       \
      rect_w = dst->w;                           \
      rect_h = dst->h;                           \
    } else {                                     \
      rect_w = dstrect->w;                       \
      rect_h = dstrect->h;                       \
      pixels += dstrect->y * width + dstrect->x; \
    }                                            \
    int seek_amount = width - rect_w;            \
    for (int i = 0; i < rect_h; ++i) {           \
      for (int j = 0; j < rect_w; ++j) {         \
        *(pixels++) = color;                     \
      }                                          \
      pixels += seek_amount;                     \
    }                                            \
  }

  if (dst->format->BitsPerPixel == 32) {
    // pixels and color is 32 bit
    uint32_t *pixels = (uint32_t *)dst->pixels;
    DO;
  } else {
    // pixels and color is 8 bit
    uint8_t *pixels = (uint8_t *)dst->pixels;
    DO;
  }
#undef DO
}

// 用NDL更新s中从[x,y]开始的size为[w,h]的rect
void SDL_UpdateRect(SDL_Surface *s, int x, int y, int w, int h) {
  // printf("SDL_UpdateRect(%x, %d, %d, %d, %d)\n", s, x, y, w, h);
  assert(s->format->BitsPerPixel == 32 || s->format->BitsPerPixel == 8);
  // CallbackHelper();
  int rect_w;
  int rect_h;
  int dst_x;
  int dst_y;
  if (x == 0 && y == 0 && w == 0 && h == 0) {
    // If 'x', 'y', 'w' and 'h' are all 0, SDL_UpdateRect will update the entire screen.
    rect_w = s->w;
    rect_h = s->h;
    dst_x = 0;
    dst_y = 0;
  } else {
    rect_w = w;
    rect_h = h;
    dst_x = x;
    dst_y = y;
  }

  // 组建要传入NDL的uint32_t pixels[rect_w * rect_h * 4]
  uint32_t *pixels = malloc((rect_w * rect_h) << 2);
  uint32_t *pixel_ptr = pixels;
  int seek_amount = s->w - rect_w;
  if (s->format->BitsPerPixel == 8) {
    uint8_t *spixel_ptr = ((uint8_t *)s->pixels) + dst_y * s->w + dst_x;
    SDL_Color *colors_ptr = s->format->palette->colors;
    for (int i = 0; i < rect_h; ++i) {
      for (int j = 0; j < rect_w; ++j) {
        SDL_Color c = colors_ptr[*(spixel_ptr++)];
        uint32_t color = c.a << 24 | c.r << 16 | c.g << 8 | c.b;
        *(pixel_ptr++) = color;
      }
      spixel_ptr += seek_amount;
    }
  } else {
    uint32_t *spixel_ptr = ((uint32_t *)s->pixels) + dst_y * s->w + dst_x;
    for (int i = 0; i < rect_h; ++i) {
      for (int j = 0; j < rect_w; ++j) {
        *(pixel_ptr++) = *(spixel_ptr++);
      }
      spixel_ptr += seek_amount;
    }
  }
  NDL_DrawRect(pixels, dst_x, dst_y, rect_w, rect_h);
  free(pixels); // am中的free什么都没做，所以完全可能程序运行一段时间后，malloc堆区把内存撑爆
}

// APIs below are already implemented.

static inline int maskToShift(uint32_t mask) {
  switch (mask) {
  case 0x000000ff:
    return 0;
  case 0x0000ff00:
    return 8;
  case 0x00ff0000:
    return 16;
  case 0xff000000:
    return 24;
  case 0x00000000:
    return 24; // hack
  default:
    assert(0);
  }
}

SDL_Surface *SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
                                  uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  assert(depth == 8 || depth == 32);
  SDL_Surface *s = malloc(sizeof(SDL_Surface));
  assert(s);
  s->flags = flags;
  s->format = malloc(sizeof(SDL_PixelFormat));
  assert(s->format);
  if (depth == 8) {
    s->format->palette = malloc(sizeof(SDL_Palette));
    assert(s->format->palette);
    s->format->palette->colors = malloc(sizeof(SDL_Color) * 256);
    assert(s->format->palette->colors);
    memset(s->format->palette->colors, 0, sizeof(SDL_Color) * 256);
    s->format->palette->ncolors = 256;
  } else {
    s->format->palette = NULL;
    s->format->Rmask = Rmask;
    s->format->Rshift = maskToShift(Rmask);
    s->format->Rloss = 0;
    s->format->Gmask = Gmask;
    s->format->Gshift = maskToShift(Gmask);
    s->format->Gloss = 0;
    s->format->Bmask = Bmask;
    s->format->Bshift = maskToShift(Bmask);
    s->format->Bloss = 0;
    s->format->Amask = Amask;
    s->format->Ashift = maskToShift(Amask);
    s->format->Aloss = 0;
  }

  s->format->BitsPerPixel = depth;
  s->format->BytesPerPixel = depth / 8;

  s->w = width;
  s->h = height;
  s->pitch = width * depth / 8;
  assert(s->pitch == width * s->format->BytesPerPixel);

  if (!(flags & SDL_PREALLOC)) {
    s->pixels = malloc(s->pitch * height);
    assert(s->pixels);
  }

  return s;
}

SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth,
                                      int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  SDL_Surface *s = SDL_CreateRGBSurface(SDL_PREALLOC, width, height, depth,
                                        Rmask, Gmask, Bmask, Amask);
  assert(pitch == s->pitch);
  s->pixels = pixels;
  return s;
}

void SDL_FreeSurface(SDL_Surface *s) {
  if (s != NULL) {
    if (s->format != NULL) {
      if (s->format->palette != NULL) {
        if (s->format->palette->colors != NULL)
          free(s->format->palette->colors);
        free(s->format->palette);
      }
      free(s->format);
    }
    if (s->pixels != NULL && !(s->flags & SDL_PREALLOC))
      free(s->pixels);
    free(s);
  }
}

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
  if (flags & SDL_HWSURFACE)
    NDL_OpenCanvas(&width, &height);
  return SDL_CreateRGBSurface(flags, width, height, bpp,
                              DEFAULT_RMASK, DEFAULT_GMASK, DEFAULT_BMASK, DEFAULT_AMASK);
}

void SDL_SoftStretch(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(src && dst);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  assert(dst->format->BitsPerPixel == 8);

  int x = (srcrect == NULL ? 0 : srcrect->x);
  int y = (srcrect == NULL ? 0 : srcrect->y);
  int w = (srcrect == NULL ? src->w : srcrect->w);
  int h = (srcrect == NULL ? src->h : srcrect->h);

  assert(dstrect);
  if (w == dstrect->w && h == dstrect->h) {
    /* The source rectangle and the destination rectangle
     * are of the same size. If that is the case, there
     * is no need to stretch, just copy. */
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_BlitSurface(src, &rect, dst, dstrect);
  } else {
    assert(0);
  }
}

void SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int firstcolor, int ncolors) {
  assert(s);
  assert(s->format);
  assert(s->format->palette);
  assert(firstcolor == 0);

  s->format->palette->ncolors = ncolors;
  memcpy(s->format->palette->colors, colors, sizeof(SDL_Color) * ncolors);

  if (s->flags & SDL_HWSURFACE) {
    assert(ncolors == 256);
    for (int i = 0; i < ncolors; i++) {
      uint8_t r = colors[i].r;
      uint8_t g = colors[i].g;
      uint8_t b = colors[i].b;
    }
    SDL_UpdateRect(s, 0, 0, 0, 0);
  }
}

static void ConvertPixelsARGB_ABGR(void *dst, void *src, int len) {
  int i;
  uint8_t(*pdst)[4] = dst;
  uint8_t(*psrc)[4] = src;
  union {
    uint8_t val8[4];
    uint32_t val32;
  } tmp;
  int first = len & ~0xf;
  for (i = 0; i < first; i += 16) {
#define macro(i)                      \
  tmp.val32 = *((uint32_t *)psrc[i]); \
  *((uint32_t *)pdst[i]) = tmp.val32; \
  pdst[i][0] = tmp.val8[2];           \
  pdst[i][2] = tmp.val8[0];

    macro(i + 0);
    macro(i + 1);
    macro(i + 2);
    macro(i + 3);
    macro(i + 4);
    macro(i + 5);
    macro(i + 6);
    macro(i + 7);
    macro(i + 8);
    macro(i + 9);
    macro(i + 10);
    macro(i + 11);
    macro(i + 12);
    macro(i + 13);
    macro(i + 14);
    macro(i + 15);
  }

  for (; i < len; i++) {
    macro(i);
  }
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, uint32_t flags) {
  assert(src->format->BitsPerPixel == 32);
  assert(src->w * src->format->BytesPerPixel == src->pitch);
  assert(src->format->BitsPerPixel == fmt->BitsPerPixel);

  SDL_Surface *ret = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel,
                                          fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

  assert(fmt->Gmask == src->format->Gmask);
  assert(fmt->Amask == 0 || src->format->Amask == 0 || (fmt->Amask == src->format->Amask));
  ConvertPixelsARGB_ABGR(ret->pixels, src->pixels, src->w * src->h);

  return ret;
}

uint32_t SDL_MapRGBA(SDL_PixelFormat *fmt, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  assert(fmt->BytesPerPixel == 4);
  uint32_t p = (r << fmt->Rshift) | (g << fmt->Gshift) | (b << fmt->Bshift);
  if (fmt->Amask)
    p |= (a << fmt->Ashift);
  return p;
}

int SDL_LockSurface(SDL_Surface *s) {
  return 0;
}

void SDL_UnlockSurface(SDL_Surface *s) {
}
