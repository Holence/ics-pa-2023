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

#include <common.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

word_t expr(char *e, bool *success);
void test_expr() {
  FILE *file = fopen("/home/hc/ics-pa-2023/nemu/tools/gen-expr/input", "r");
  if (file == NULL) {
    perror("Error opening file");
    return;
  }

  word_t reference;
  char expression[32];

  int total = 0;
  int correct = 0;
  bool success = true;
  while (fscanf(file, "%u %[^\n]", &reference, expression) == 2) {
    printf(ANSI_FMT("%s = %u\n", ANSI_FG_BLACK), expression, reference);
    total++;
    word_t result = expr(expression, &success);
    if (result != reference) {
      printf(ANSI_FMT("%u - Not Match\n", ANSI_FG_RED), result);
      getc(stdin);
    } else {
      correct++;
      printf(ANSI_FMT("Match\n", ANSI_FG_GREEN));
    }
  }
  fclose(file);

  printf(ANSI_FMT("Total Test: %d\nCorrect: %d\n", ANSI_FG_MAGENTA), total, correct);
}

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  // test expr in PA 1.4
  // test_expr();

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
