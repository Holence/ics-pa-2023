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
  }

#define digit_length(x) \
  ({                    \
    int length = 0;     \
    int temp = x;       \
    while (temp > 0) {  \
      temp = temp / 10; \
      length++;         \
    }                   \
    length;             \
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
        int number = va_arg(ap, int);
        if (number < 0) {
          write_char('-');
          number = -number;
        }
        int length = digit_length(number);
        for (int i = length - 1; i >= 0; i--) {
          out[i] = number % 10 + '0';
          number = number / 10;
        }
        out = out + length;
        translate = false;
      };
      break;
    case 's':
      char *s = va_arg(ap, char *);
      strcpy(out, s);
      out = out + strlen(s);
      translate = false;
      break;
    default:
      *out = *fmt;
      out++;
      char_written++;
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
