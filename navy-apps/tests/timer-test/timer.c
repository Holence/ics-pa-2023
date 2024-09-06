#include <stdio.h>
#include <NDL.h>

int main() {
  uint32_t last_tick = NDL_GetTicks();
  printf("%d\n", last_tick);
  while (1) {
    while (1) {
      uint32_t tick = NDL_GetTicks();
      if (tick > last_tick) {
        printf("time up %d\n", last_tick);
        last_tick = tick + 500;
        break;
      }
    }
  }
  return 0;
}
