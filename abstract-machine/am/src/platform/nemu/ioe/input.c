#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t key = inl(KBD_ADDR);
  if ((key & KEYDOWN_MASK) == KEYDOWN_MASK) {
    kbd->keydown = true;
    kbd->keycode = key & 0x00FF;
  } else {
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
  }
}
