#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define __KLIB_UINT uint64_t
#define __KLIB_INT  int64_t

#define write_char(c) \
  {                   \
    *out = c;         \
    out++;            \
    char_written++;   \
  }

#define digit_length(x, base) \
  ({                          \
    int length = 0;           \
    __KLIB_UINT temp = x;     \
    while (temp > 0) {        \
      temp = temp / base;     \
      length++;               \
    }                         \
    length;                   \
  })

int write_number(char *out, __KLIB_UINT number, int base) {
  int char_written = 0;
  if (number == 0) {
    write_char('0');
    return char_written;
  }

  int length = digit_length(number, base);
  for (int i = length - 1; i >= 0; i--) {
    switch (base) {
    case 10:
      out[i] = number % base + '0';
      number = number / base;
      break;
    case 16:
      uint8_t digit = number & 0b1111;
      if (digit < 10) {
        out[i] = digit + '0';
      } else {
        out[i] = digit - 10 + 'A';
      }
      number = number >> 4;
      break;
    default:
      break;
    }
  }
  char_written += length;
  return char_written;
}

#define digit_case                          \
  length = write_number(out, number, base); \
  char_written += length;                   \
  out = out + length;                       \
  break;

int vsprintf(char *out, const char *fmt, va_list ap) {
  int char_written = 0;
  int base;
  int length;
  bool translate = false;
  __KLIB_UINT number;
  while (*fmt != '\0') {
    if (*fmt == '%') {
      if (translate) {
        write_char('%');
        translate = false;
      } else {
        translate = true;
      }
    } else {
      if (translate) {
        translate = false;
        switch (*fmt) {
        case 'd':
          base = 10;
          __KLIB_INT temp = (__KLIB_INT)va_arg(ap, int);
          // assert()
          number = temp;
          if (temp < 0) {
            write_char('-');
            // INT_MIN取负 是 Undefined Behavior
            // 但编译时的数值会被Woverflow警告，而运行时变量里的数值一定在正确的范围内
            // 所以这里应该没有问题
            number = -temp;
          }
          digit_case;
        case 'p':
          write_char('0');
          write_char('x');
          base = 16;
          number = (__KLIB_UINT)va_arg(ap, int);
          digit_case;
        case 'x':
          base = 16;
          number = (__KLIB_UINT)va_arg(ap, int);
          digit_case;
        case 's':
          char *s = va_arg(ap, char *);
          assert(s != NULL);
          strcpy(out, s);
          length = strlen(s);
          out = out + length;
          char_written += length;
          break;
        case 'c':
          int c = va_arg(ap, int);
          write_char((uint8_t)c);
          break;
        default:
          putch(*fmt);
          putch('\n');
          panic("vsprintf not recognize format");
        }
      } else {
        write_char(*fmt);
      }
    }
    fmt++;
  }
  *out = '\0';
  return char_written;
}

#define MAX_PRINT_LEN 2048
static char buf[MAX_PRINT_LEN];
int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int char_written = vsprintf(buf, fmt, ap);
  va_end(ap);
  if (char_written > MAX_PRINT_LEN) {
    panic("printf's output should not longer than " TOSTRING(MAX_PRINT_LEN) " chars");
  }
  for (int i = 0; i < char_written; i++) {
    putch(buf[i]);
  }
  return char_written;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int char_written = vsprintf(out, fmt, ap);
  va_end(ap);
  return char_written;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int char_written = vsprintf(out, fmt, ap);
  va_end(ap);
  if (char_written + 1 > n) {
    out[n] = '\0';
  }
  return char_written;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
