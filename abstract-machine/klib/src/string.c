#include <klib-macros.h>
#include <klib.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s != '\0') {
    len++;
    s++;
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

char *strncpy(char *dst, const char *src, size_t n) {
  size_t len = strlen(dst);
  size_t copy_len = strnlen(src, n);
  memcpy(dst, src, copy_len);
  if (copy_len < len) {
    memset(dst + copy_len, 0, len - copy_len);
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
    n--;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  char *temp = malloc(n);
  memcpy(temp, src, n);
  memcpy(dst, src, n);
  free(temp);
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
    cmp = *ptr1 - *ptr1;
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
