#include <klib-macros.h>
#include <klib.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

size_t strnlen(const char *s, size_t maxlen) {
  size_t len = strlen(s);
  return len < maxlen ? len : maxlen;
}

char *strcpy(char *dst, const char *src) {
  size_t len = strlen(src);
  memcpy(dst, src, len);
  *(dst + len) = '\0';
  return dst;
}

/**
 * num > len(src), pad zero to total of num
 * num < len(src), do not add \0 in the end
 */
char *strncpy(char *dst, const char *src, size_t num) {
  size_t copy_len = strnlen(src, num);
  memcpy(dst, src, copy_len);
  if (copy_len < num) {
    memset(dst + copy_len, 0, num - copy_len);
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  strcpy(dst + strlen(dst), src);
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (true) {
    int cmp = *s1 - *s2;
    if (*s1 == '\0' || *s2 == '\0') {
      return cmp;
    }
    if (cmp != 0) {
      return cmp;
    }
    s1++;
    s2++;
  }
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) {
    return 0;
  }
  int cmp;
  while (n > 0) {
    cmp = *s1 - *s2;
    if (*s1 == '\0' || *s2 == '\0') {
      break;
    }
    if (cmp != 0) {
      break;
    }
    s1++;
    s2++;
    n--;
  }
  return cmp;
}

void *memset(void *s, int c, size_t n) {
  uint8_t one_byte = c;
  uint8_t *ptr = s;
  while (n > 0) {
    *ptr = one_byte;
    ptr++;
    n--;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  // malloc也是klib中需要自己实现的部分，还没实现的时候不能用！
  // char *temp = malloc(n);
  // memcpy(temp, src, n);
  // memcpy(dst, temp, n);
  // free(temp);
  // return dst;

  if (n == 0)
    return dst;

  const char *s = src;
  char *d = dst;

  if (s < d && s + n > d) {
    // 只有这一种情况需要逆向复制
    // src: -----
    // dst:   -----
    s += n;
    d += n;
    while (n-- > 0) {
      *--d = *--s;
    }
  } else
    // 正向逆向都行
    // src: -----
    // dst:        -----
    // 只能正向
    // src:   -----
    // dst: -----
    while (n-- > 0) {
      *d++ = *s++;
    }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  uint8_t *ptr_out = (uint8_t *)out;
  uint8_t *ptr_in = (uint8_t *)in;
  while (n > 0) {
    *ptr_out = *ptr_in;
    ptr_in++;
    ptr_out++;
    n--;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  if (n == 0) {
    return 0;
  }
  uint8_t *ptr1 = (uint8_t *)s1;
  uint8_t *ptr2 = (uint8_t *)s2;
  int cmp;
  while (n > 0) {
    cmp = *ptr1 - *ptr2;
    if (cmp != 0) {
      break;
    }
    ptr1++;
    ptr2++;
    n--;
  }
  return cmp;
}

#endif
