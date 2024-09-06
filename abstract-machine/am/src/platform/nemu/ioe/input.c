#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t key = inl(KBD_ADDR);
  if (key != AM_KEY_NONE) {
    kbd->keydown = key & KEYDOWN_MASK;
    kbd->keycode = key & 0x00FF;
  } else {
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
  }
}
