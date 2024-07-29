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

  // 这些数字会在计算时用于优先级判断
  TK_ADD = 1,
  TK_SUB = 2,
  TK_MUL = 3,
  TK_DIV = 4,

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

    {"\\+", TK_ADD},
    {"-", TK_SUB},
    {"\\*", TK_MUL},
    {"/", TK_DIV},

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

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);

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
        case TK_ADD:
        case TK_SUB:
        case TK_MUL:
        case TK_DIV:
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

bool is_full_parentheses(int p, int q) {
  if (tokens[p].type == '(' && tokens[q].type == ')') {
    bool a_pair_complete = false;
    int count_parentheses = 0;
    for (int i = p; i <= q; i++) {

      if (tokens[i].type == '(') {
        if (a_pair_complete) {
          // ( ) ( )
          return false;
        }
        count_parentheses++;
      }

      if (tokens[i].type == ')') {
        count_parentheses--;
        if (count_parentheses == 0) {
          a_pair_complete = true;
        }
      }

      if (count_parentheses < 0) {
        // ( ) )
        return false;
      }
    }

    if (count_parentheses > 0) {
      // ( ( )
      return false;
    }
    return true;
  } else {
    return false;
  }
}

bool has_valid_parentheses(int p, int q) {
  int count_parentheses = 0;
  for (int i = p; i <= q; i++) {

    if (tokens[i].type == '(') {
      count_parentheses++;
    }

    if (tokens[i].type == ')') {
      count_parentheses--;
    }

    if (count_parentheses < 0) {
      // ( ) )
      return false;
    }
  }

  if (count_parentheses > 0) {
    // ( ( )
    return false;
  }
  return true;
}

int get_main_op_index(int p, int q) {
  // 非运算符的token不是主运算符.
  // 出现在一对括号中的token不是主运算符. 注意到这里不会出现有括号包围整个表达式的情况, 因为这种情况已经在check_parentheses()相应的if块中被处理了.
  // 主运算符的优先级在表达式中是最低的. 这是因为主运算符是最后一步才进行的运算符.
  // 当有多个运算符的优先级都是最低时, 根据结合性, 最后被结合的运算符才是主运算符. 一个例子是1 + 2 + 3, 它的主运算符应该是右边的+.

  int op_index = p + 1; // 直接从第二个开始, 为了让 "-1 * 2" 的找到 * 而不是 -
  int min_priority = TK_DIV;
  bool found = false;                             // -(1+2) 这种算是没有main op
  int inside_parentheses = tokens[p].type == '('; // -( (1) + 1 ) 可能有多重嵌套，所以得用count来数
  for (int i = p + 1; i <= q; i++) {
    int token_type = tokens[i].type;
    if (token_type != TK_NUM) {
      if (token_type == '(') {
        inside_parentheses++;
        continue;
      } else if (token_type == ')') {
        inside_parentheses--;
        continue;
      }

      if (inside_parentheses == 0) {
        if (token_type <= min_priority) {
          // 不是数字，不在括号内，且优先级最低，且靠最右的
          found = true;
          // 加减同级
          if (token_type <= TK_SUB) {
            min_priority = TK_SUB;
          }
          op_index = i;
        }
      }
    }
  }
  if (found) {
    return op_index;
  } else {
    return -1;
  }
}

word_t eval(int p, int q) {
  // if (q < 0 || p >= nr_token) {
  if (p > q) {
    /* Bad expression
      "3 + " will call eval(0, 0) and eval (2, 1)
      " + 3" will call eval(0, -1) and eval (1, 1)
     */
    bad_expression = true;
    return 0;
  } else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (tokens[p].type == TK_NUM) {

      return atoi(tokens[p].str);
    } else {
      bad_expression = true;
      return 0;
    }
  } else if (is_full_parentheses(p, q)) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  } else {
    // is_full_parentheses() == false, invalid expression
    // (4 + 3)) * ((2 - 1)
    if (!has_valid_parentheses(p, q)) {
      bad_expression = true;
      return 0;
    }

    // is_full_parentheses() == false, but has_valid_parentheses() == true
    // (4 + 3) * (2 - 1)
    // 4 + 3 * 1
    // -4 + 3 * 1
    // -(4 + 3 * 1)

    int op_index = get_main_op_index(p, q);

    // not have main op
    // starting with +/-
    // +NUMBER / +(1)
    // -(-(+1))
    if (op_index == -1) {
      if (tokens[p].type == TK_ADD) {
        return eval(p + 1, q);
      } else if (tokens[p].type == TK_SUB) {
        return -eval(p + 1, q);
      } else {
        bad_expression = true;
        return 0;
      }
    }

    // have main op
    // -(-1+1) + 1
    /*
     * 为了正确地计算负数的乘法除法，这里 val1, val2 先用int表示，最后返回到expr()时会被解释为word_t
     * -6 / 4 = -1 ，而不是 4294967290 / 4 = 1073741822
     */
    int val1 = eval(p, op_index - 1);
    if (bad_expression)
      return 0;

    int val2 = eval(op_index + 1, q);
    if (bad_expression)
      return 0;

    switch (tokens[op_index].type) {
    case TK_ADD:
      return val1 + val2;
    case TK_SUB:
      return val1 - val2;
    case TK_MUL:
      return val1 * val2;
    case TK_DIV:
      return val1 / val2;
    default:
      panic("Not support op: %d", tokens[op_index].type);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* Insert codes to evaluate the expression. */
  bad_expression = false;
  word_t result = eval(0, nr_token - 1);
  if (bad_expression) {
    *success = false;
  }
  return result;
}
