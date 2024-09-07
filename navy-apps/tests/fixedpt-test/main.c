#include <stdio.h>
#include <fixedptc.h>

void test(fixedpt A) {
  printf("A: %s 0x%x\n", fixedpt_cstr(A, -1), A);
  printf("abs(A): %s\n", fixedpt_cstr(fixedpt_abs(A), -1));
  printf("floor(A): %s\n", fixedpt_cstr(fixedpt_floor(A), -1));
  printf("ceil(A): %s\n", fixedpt_cstr(fixedpt_ceil(A), -1));
  printf("A/3: %s\n", fixedpt_cstr(fixedpt_divi(A, 3), -1));
  printf("A*3: %s\n", fixedpt_cstr(fixedpt_muli(A, 3), -1));
  printf("A/-3: %s\n", fixedpt_cstr(fixedpt_divi(A, -3), -1));
  printf("A*-3: %s\n", fixedpt_cstr(fixedpt_muli(A, -3), -1));
  printf("\n");

  fixedpt B = fixedpt_rconst(2.1);
  printf("B: %s 0x%x\n", fixedpt_cstr(B, -1), B);
  printf("A+B: %s\n", fixedpt_cstr(fixedpt_add(A, B), -1));
  printf("A-B: %s\n", fixedpt_cstr(fixedpt_sub(A, B), -1));
  printf("A*B: %s\n", fixedpt_cstr(fixedpt_mul(A, B), -1));
  printf("A/B: %s\n", fixedpt_cstr(fixedpt_div(A, B), -1));
  printf("\n");

  B = fixedpt_rconst(-2.1);
  printf("B: %s 0x%x\n", fixedpt_cstr(B, -1), B);
  printf("A+B: %s\n", fixedpt_cstr(fixedpt_add(A, B), -1));
  printf("A-B: %s\n", fixedpt_cstr(fixedpt_sub(A, B), -1));
  printf("A*B: %s\n", fixedpt_cstr(fixedpt_mul(A, B), -1));
  printf("A/B: %s\n", fixedpt_cstr(fixedpt_div(A, B), -1));
  printf("\n---------------\n");
}

int main() {
  test(fixedpt_rconst(1.55));
  test(fixedpt_rconst(-1.55));
  return 0;
}
