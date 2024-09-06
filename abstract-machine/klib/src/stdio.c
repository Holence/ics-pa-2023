#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

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

#if __riscv_xlen == 64
#define __KLIB_UINT uint64_t
#define __KLIB_INT int64_t
#else
#define __KLIB_UINT uint32_t
#define __KLIB_INT int32_t
#endif

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

int vsprintf(char *out, const char *fmt, va_list ap) {
  int char_written = 0;
  int base;
  bool translate = false;
  __KLIB_UINT number;
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
        base = 10;
        __KLIB_INT temp = (__KLIB_INT)va_arg(ap, int);
        number = temp;
        if (temp < 0) {
          write_char('-');
          number = -temp; // ❓INT_MIN被取负是Undefined Behavior，谁知道哪天就出bug了，但想不出更好的办法了
        }
        goto digit_case;
      }
    case 'p':
      if (translate) {
        write_char('0');
        write_char('x');
        base = 16;
        number = (__KLIB_UINT)va_arg(ap, int);
        goto digit_case;
      }
    case 'x':
      if (translate) {
        base = 16;
        number = (__KLIB_UINT)va_arg(ap, int);
        goto digit_case;
      }
    case 's':
      if (translate) {
        translate = false;
        char *s = va_arg(ap, char *);
        assert(s != NULL);
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
      translate = false; // 如果前面是%，后面遇到不认识的格式（这里还没实现的格式），取消本次转义
      write_char(*fmt);
      break;
    digit_case:
      translate = false;
      int length = write_number(out, number, base);
      char_written += length;
      out = out + length;
      break;
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
