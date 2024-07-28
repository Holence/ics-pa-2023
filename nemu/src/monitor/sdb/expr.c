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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256,
  TK_EQ,

  /* Add more token types */
  TK_NUM,

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

    /* Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE}, // spaces
    {"==", TK_EQ},     // equal

    {"\\+", '+'}, // plus
    {"-", '-'},   // minus
    {"\\*", '*'}, // mul
    {"/", '/'},   // div

    {"[0-9]+", TK_NUM}, // number
    {"\\(", '('},       // (
    {"\\)", ')'},       // )

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  // compile all regex rules
  for (i = 0; i < NR_REGEX; i++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
        case TK_NOTYPE:
          break;
        case TK_NUM: {
          if (substr_len < 32) {
            strcpy(tokens[nr_token].str, substr_start);
          } else {
            panic("token not less than 32 char: %s", substr_start);
          }
        }
        case TK_EQ:
        case '+':
        case '-':
        case '*':
        case '/':
        default:
          tokens[nr_token].type = rules[i].token_type;
          nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool bad_expression;

bool check_parentheses(int p, int q) {
  // TODO
  return true;
}

word_t eval(int p, int q) {
  if (p > q) {
    /* Bad expression
      "3 + " will call eval(0, 0) and eval (2, 1)
     */
    bad_expression = true;
    return 0;
  } else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    return atoi(tokens[q].str);
  } else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  } else {
    // TODO
    // op = the position of 主运算符 in the token expression;
    int op = 0;

    word_t val1 = eval(p, op - 1);
    if (bad_expression)
      return 0;

    word_t val2 = eval(op + 1, q);
    if (bad_expression)
      return 0;

    // switch (op_type) {
    // case '+':
    //   return val1 + val2;
    // case '-': /* ... */
    // case '*': /* ... */
    // case '/': /* ... */
    // default:
    //   assert(0);
    // }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* Insert codes to evaluate the expression. */
  bad_expression = false;
  word_t result = eval(0, nr_token);
  if (bad_expression) {
    *success = false;
  }
  return result;
}
