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

#include "sdb.h"
#include <cpu/cpu.h>
#include <isa.h>
#include <memory/vaddr.h>
#include <readline/history.h>
#include <readline/readline.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1); // -1 in uint64_t is 18446744073709551615
  return 0;
}

static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT; // PA1. RTFSC, 优美地退出
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_px(char *args);
// static int cmd_w(char *args);
// static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands\n      help [cmd]", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},

    /* Add more commands */
    {"si", "Step by machine instructions\n      si [N]: step N instructions, N default to 1", cmd_si},
    {"info", "Show info\n      info r: to print registers\n      info w: to print watchpoints", cmd_info},
    {"x", "Examine memory at address expr\n      x N EXPR: print N*4 bytes after M[EXPR], N is a positive int", cmd_x},
    {"p", "Show value of expr in decimal\n      p EXPR", cmd_p},
    {"px", "Show value of expr in hexadecimal\n      px EXPR", cmd_px},
    // {"w", "Set a watchpoint for expression expr", cmd_w},
    // {"d", "Delete watchpoint", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

bool is_args_empty(char *s) {
  if (s == NULL)
    return true;
  while (s[0] == ' ')
    s++;
  if (s[0] == '\0')
    return true;
  else
    return false;
}

static int cmd_help(char *arg) {
  /* extract the first argument */
  // NULL means to continue tokenizing the string you passed in previous call to strtok()
  int i;

  if (is_args_empty(arg)) {
    /* no argument given */
    // print all cmd info
    for (i = 0; i < NR_CMD; i++) {
      printf("%-4s: %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  } else {
    // print the named one
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%-4s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf(ANSI_FMT("Unknown command '%s'\n", ANSI_FG_RED), arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  int N = 1;
  if (arg != NULL) {
    N = atoi(arg); // If the input string is not a valid string's number, it returns 0.
  }
  cpu_exec(N);
  return 0;
}

static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    /* no argument given */
    return 1;
  } else {
    if (strcmp(arg, "r") == 0) {
      isa_reg_display();
    } else if (strcmp(arg, "w") == 0) {
      printf("info watchpoint\n");
    } else {
      return 1;
    }
  }
  return 0;
}

static int cmd_x(char *args) {
  int arg_str_len = strlen(args);
  char *first = strtok(NULL, " ");
  if (first == NULL) {
    /* no argument given */
    return 1;
  }

  int N = atoi(first);
  if (N == 0 || arg_str_len == strlen(first)) {
    /* no EXPR given */
    return 1;
  }

  char *second = args + strlen(first) + 1;
  if (is_args_empty(second)) {
    /* EXPR is empty */
    return 1;
  }

  bool success;
  vaddr_t base_address = expr(second, &success);
  if (success) {
    // print format
    // address   : value      value      value      value
    // 0x80000000: 0x00000001 0x00000002 0x00000003 0x00000004
    // 0x80000010: 0x00000005 0x00000006
    int i;
    for (i = 0; i < N; i++) {
      if (i % 4 == 0) {
        printf(ANSI_FMT("0x%8x: ", ANSI_FG_CYAN), base_address);
      }
      printf("0x%08x ", vaddr_read(base_address, sizeof(word_t)));
      base_address += 4;
      if (i % 4 == 3) {
        printf("\n");
      }
    }
    if (i % 4 != 0) {
      printf("\n");
    }
  } else {
    printf(ANSI_FMT("Invalid EXPR\n", ANSI_FG_RED));
  }

  return 0;
}

static int cmd_p(char *args) {
  if (is_args_empty(args)) {
    return 1;
  }

  bool success;
  word_t result = expr(args, &success);
  if (success) {
    printf("%u\n", result);
  } else {
    printf(ANSI_FMT("Invalid EXPR\n", ANSI_FG_RED));
  }
  return 0;
}

static int cmd_px(char *args) {
  if (is_args_empty(args)) {
    return 1;
  }

  bool success;
  word_t result = expr(args, &success);
  if (success) {
    printf("0x%08x\n", result);
  } else {
    printf(ANSI_FMT("Invalid EXPR\n", ANSI_FG_RED));
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  // main loop
  for (char *str; (str = rl_gets()) != NULL;) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     * args is the starting address of "args"
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        int ret = cmd_table[i].handler(args);

        // ret == 1, invalid cmd format
        if (ret == 1) {
          printf(ANSI_FMT("Invalid cmd format\n", ANSI_FG_RED));
          cmd_help(cmd);
        }

        // ret == -1, quit cmd
        else if (ret == -1) {
          return; // exit main loop
        }

        // ret == 0, continue
        break;
      }
    }

    if (i == NR_CMD) {
      printf(ANSI_FMT("Unknown command '%s'\n", ANSI_FG_RED), cmd);
    }
  }
  // main loop
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
