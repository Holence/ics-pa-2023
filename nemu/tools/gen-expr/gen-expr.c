/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// this should be enough
static char expr[65536] = {};
static int expr_index = 0;
static char code_buf[65536 + 128] = {}; // a little larger than `expr`
static char *code_format =
    "#include <signal.h>\n"
    "#include <stdio.h>\n"
    "#include <stdlib.h>\n"
    "void handler(int sig) { exit(1); }\n"
    "int main() { "
    "  signal(SIGFPE, handler);" // divide by zero, will call handler and exit(1)
    "  unsigned result = %s; "
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}";

static int choose(int max) {
  return rand() % max;
}

static void gen(char c) {
  if (choose(20) == 0) {
    expr[expr_index++] = ' '; // random add space
  }
  expr[expr_index++] = c;
  if (choose(20) == 0) {
    expr[expr_index++] = ' '; // random add space
  }
}

static void gen_num() {
  gen(choose(10) + '0');
}

static void gen_rand_op() {
  switch (choose(4)) {
  case 0:
    gen('+');
    break;
  case 1:
    gen('-');
    break;
  case 2:
    gen('*');
    break;
  case 3:
    gen('/');
    break;
  }
}
void gen_pos_neg() {
  switch (choose(4)) {
  case 0:
    gen('+');
    break;
  case 1:
    gen('-');
    break;
  default:
    break;
  }
}

static void gen_rand_expr() {
  // if expr is shorter than 32 char, quit generating
  if (expr_index > 32) {
    return;
  }
  int ch = choose(100);
  if (ch < 40) {
    gen_num();
  } else if (ch < 80) {
    gen('(');
    gen_pos_neg();
    gen_rand_expr();
    gen(')');
  } else {
    gen_rand_expr();
    gen_rand_op();
    gen_rand_expr();
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i++) {

    expr_index = 0;
    gen_pos_neg();
    gen_rand_expr();
    expr[expr_index] = '\0';
    // if expr is shorter than 32 char, expr is accepted
    if (expr_index > 32) {
      // printf("%s\nlonger than 32 char\n", expr);
      continue;
    }

    // generate code
    sprintf(code_buf, code_format, expr);

    // write code to temp file
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // compile code
    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) {
      // printf("%s\ndivide by zero\n", expr);
      continue;
    }

    // run
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int calc_result;
    fscanf(fp, "%d", &calc_result);
    ret = pclose(fp);

    // if return value is not 1, result is accepted
    if (ret == 0) {
      printf("%u %s\n", calc_result, expr);
    }
  }
  return 0;
}
