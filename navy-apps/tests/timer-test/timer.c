
#include <stdio.h>
#include <sys/time.h>

int _gettimeofday(struct timeval *tv, struct timezone *tz);

int main() {
  struct timeval t;
  _gettimeofday(&t, NULL);
  printf("%ld %ld\n", t.tv_sec, t.tv_usec);
  int last_usec = 0;
  while (1) {
    while (1) {
      _gettimeofday(&t, NULL);
      if (t.tv_usec > last_usec && t.tv_usec - last_usec > 500000) {
        last_usec = t.tv_usec;
        break;
      } else if (t.tv_usec < last_usec && 1000000 - last_usec + t.tv_usec > 500000) {
        last_usec = t.tv_usec;
        break;
      }
    }
    printf("time up % ld % ld\n", t.tv_sec, t.tv_usec);
  }
  return 0;
}
