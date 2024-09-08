#include <stdio.h> 
#include <stdint.h> 
#include <stdlib.h>
#include <assert.h>

struct BitmapHeader {
  uint16_t type;
  uint32_t filesize;
  uint32_t resv_1;
  uint32_t offset;
  uint32_t ih_size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bitcount; // 1, 4, 8, or 24
  uint32_t compression;
  uint32_t sizeimg;
  uint32_t xres, yres;
  uint32_t clrused, clrimportant;
} __attribute__((packed));

void* BMP_Load(const char *filename, int *width, int *height) {
  FILE *fp = fopen(filename, "r");
  if (!fp) return NULL;

  struct BitmapHeader hdr;
  assert(sizeof(hdr) == 54);
  assert(1 == fread(&hdr, sizeof(struct BitmapHeader), 1, fp));

  if (hdr.bitcount != 24) return NULL;
  if (hdr.compression != 0) return NULL;
  int w = hdr.width;
  int h = hdr.height;
  uint32_t *pixels = malloc(w * h * sizeof(uint32_t));

  // Row padding: each row in a BMP fp is padded to a multiple of 4 bytes
  // int rowPadding = (4 - (w * 3) % 4) % 4;
  fseek(fp, hdr.offset, SEEK_SET);
  uint32_t *pixel_ptr = pixels + (h - 1) * w;
  int two_w = w << 1;
  // Read pixel data row by row (BMP stores pixels bottom-up)
  // 从上往下读bmp文件，同时从下往上写入pixel
  uint8_t *buffer = malloc(w * 3 * sizeof(uint8_t));
  for (int y = h - 1; y >= 0; y--) {
    fread(buffer, 3, w, fp);
    uint8_t *buffer_ptr = buffer;
    // 由于slide的bmp是24bit的，还得一个一个扩充成32bit
    for (int x = 0; x < w; x++) {
      // Read 3 bytes (B, G, R)
      uint8_t blue = *(buffer_ptr++), green = *(buffer_ptr++), red = *(buffer_ptr++);
      // Store the pixel in the format 0x00RRGGBB
      *(pixel_ptr++) = (red << 16) | (green << 8) | blue;
    }
    pixel_ptr -= two_w;
    // Skip row padding if any
    // fseek(fp, rowPadding, SEEK_CUR);
  }
  free(buffer);

  fclose(fp);
  if (width) *width = w;
  if (height) *height = h;
  return pixels;
}
