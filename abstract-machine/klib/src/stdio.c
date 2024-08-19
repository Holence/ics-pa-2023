#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#define write_char(c) \
  {                   \
    *out = c;         \
    out++;            \
    char_written++;   \
  }

#define BIGGER_INT int64_t
#define digit_length(x)  \
  ({                     \
    int length = 0;      \
    BIGGER_INT temp = x; \
    while (temp > 0) {   \
      temp = temp / 10;  \
      length++;          \
    }                    \
    length;              \
  })

int sprintf(char *out, const char *fmt, ...) {
  int char_written = 0;
  va_list ap;
  va_start(ap, fmt);
  bool translate = false;
  while (*fmt != '\0') {
    switch (*fmt) {
    case '%':
      if (translate) {
        write_char('%');
        translate = false;
      } else {
        translate = true;
      }
      break;
    case 'd':
      if (translate) {
        translate = false;
        BIGGER_INT number = (BIGGER_INT)va_arg(ap, int); // 不是很好的方法，只是把上限提高，以保证最小的负数能成功翻转成正数
        if (number == 0) {
          write_char('0');
          break;
        }
        if (number < 0) {
          write_char('-');
          number = -number;
        }
        int length = digit_length(number);
        for (int i = length - 1; i >= 0; i--) {
          out[i] = number % 10 + '0';
          number = number / 10;
        }
        char_written += length;
        out = out + length;

        break;
      } // to default
    case 's':
      if (translate) {
        translate = false;
        char *s = va_arg(ap, char *);
        strcpy(out, s);
        int length = strlen(s);
        out = out + length;
        char_written += length;
        break;
      } // to default
    case 'c':
      if (translate) {
        translate = false;
        int c = va_arg(ap, int);
        write_char((uint8_t)c);
        break;
      } // to default
    default:
      write_char(*fmt);
      break;
    }
    fmt++;
  }
  *out = '\0';
  va_end(ap);
  return char_written;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
