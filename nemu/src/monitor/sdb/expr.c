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
#include <memory/vaddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {

  // 这些数字会在计算时用于优先级判断
  TK_NOTYPE = 0,       // space
  TK_L_Parenthese = 2, // (
  TK_R_Parenthese = 3, // )
  TK_REG = 7,          // number
  TK_HEX = 8,          // number
  TK_NUM = 9,          // number

  // Unary
  TK_POS = 10,   // +1
  TK_NEG = 11,   // -1
  TK_DEREF = 12, // *var

  // Binary
  TK_MUL = 20, // 1 * 1
  TK_DIV = 21, // 1 / 1

  TK_ADD = 30, // 1 + 1
  TK_SUB = 31, // 1 - 1

  TK_EQ = 40,  // 1 == 1
  TK_NEQ = 41, // 1 != 1

  TK_AND = 50, // 1 && 1
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

    /* Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE},        // spaces
    {"\\(", TK_L_Parenthese}, // (
    {"\\)", TK_R_Parenthese}, // )

    {"\\+", TK_ADD}, // ADD or POS
    {"-", TK_SUB},   // SUB or NEG
    {"\\*", TK_MUL}, // MUL or DEREF
    {"/", TK_DIV},

    {"==", TK_EQ},  // equal
    {"!=", TK_NEQ}, // equal
    {"&&", TK_AND}, // and

    {"\\$[0-9a-z]+", TK_REG},   // $reg
    {"0x[0-9A-Fa-f]+", TK_HEX}, // hex number
    {"[0-9]+", TK_NUM},         // number c的正则库里竟然连\d都不支持……
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
          // space is omitted
          break;

        case TK_REG:
        case TK_HEX:
        case TK_NUM: {
          if (substr_len < 32) {
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            // if source is less than dest, must manually add '\0'
            tokens[nr_token].str[substr_len] = '\0';
          } else {
            panic("token not less than 32 char: %s", substr_start);
          }
        }
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

bool is_full_parentheses(int p, int q) {
  if (tokens[p].type == TK_L_Parenthese && tokens[q].type == TK_R_Parenthese) {
    bool a_pair_complete = false;
    int count_parentheses = 0;
    for (int i = p; i <= q; i++) {

      if (tokens[i].type == TK_L_Parenthese) {
        if (a_pair_complete) {
          // ( ) ( )
          return false;
        }
        count_parentheses++;
      }

      if (tokens[i].type == TK_R_Parenthese) {
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

    if (tokens[i].type == TK_L_Parenthese) {
      count_parentheses++;
    }

    if (tokens[i].type == TK_R_Parenthese) {
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

  // +1*2 + 3/2 - 1 + 1     这里的第一个是TK_POS，main op是最后面的'+'
  // -(1+1) + (-1)          这里的第一个是TK_NEG，main op 是两组括号之间的'+'
  // -(1+1)                 op_index 将等于 p
  // eval()中会通过判断返回的op_index是否等于p，来决定是unary操作还是binary操作

  int op_index = p;
  int max_priority = TK_MUL;
  int inside_parentheses = 0; //  ((1) + 1) - 1  可能有多重嵌套，所以得用count来数
  for (int i = p; i <= q; i++) {
    int token_type = tokens[i].type;
    if (token_type == TK_L_Parenthese) {
      inside_parentheses++;
      continue;
    } else if (token_type == TK_R_Parenthese) {
      inside_parentheses--;
      continue;
    }
    if (token_type > TK_NUM) {
      if (inside_parentheses == 0) {
        if (token_type >= max_priority) {
          // 不是数字，不在括号内，且优先级最低，且靠最右的
          max_priority = (token_type / 10) * 10; // 同级的归类
          // 比如TK_SUB为21，和20的TK_ADD同级，max_priority应该记录20而不是21
          op_index = i;
        }
      }
    }
  }
  return op_index;
}

static bool bad_expression;

word_t eval(int p, int q) {
  if (p > q) {
    /* Bad expression
      "3 + " will call eval(0, 0) and eval (2, 1)
     */
    bad_expression = true;
    return 0;
  } else if (p == q) {
    /* Single token.
     * $reg / 0x1A / 14
     */
    switch (tokens[p].type) {
    case TK_REG:
      bool success;
      word_t reg_value = isa_reg_str2val(tokens[p].str + 1, &success); // +1 means skip the starting '$'
      if (!success) {
        printf(ANSI_FMT("Cannot find register named \"%s\"\n", ANSI_FG_RED), tokens[p].str + 1);
        bad_expression = true;
        return 0;
      } else {
        return reg_value;
      }
    case TK_HEX:
      return strtol(tokens[p].str, NULL, 16);
    case TK_NUM:
      return atoi(tokens[p].str);
    default:
      bad_expression = true;
      return 0;
    }
  } else if (is_full_parentheses(p, q)) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     * (...) yes
     * (...) + (...) no
     * ((...) + (...)) yes
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

    // not have main op, Unary structure
    // starting with +/-/*
    // +NUMBER / +(1) / *var / *(var+1)
    // -(-(+1))
    if (op_index == p) {
      if (tokens[p].type == TK_POS) {
        return eval(p + 1, q);
      } else if (tokens[p].type == TK_NEG) {
        return -eval(p + 1, q);
      } else if (tokens[p].type == TK_DEREF) {
        return vaddr_read(eval(p + 1, q), sizeof(word_t));
      } else {
        panic("Why are you here?? eval() op_index==q");
      }
    }

    // have main op, Binary structure
    // -4 + 3 * 1
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
    case TK_EQ:
      return val1 == val2;
    case TK_NEQ:
      return val1 != val2;
    case TK_AND:
      return val1 && val2;
    default:
      panic("Not support op: %d", tokens[op_index].type);
    }
  }
}

word_t expr(char *e, bool *success) {
  *success = true;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* Insert codes to evaluate the expression. */

  // convert to unary
  // +1 / -1 / *var
  // (+1) / (-1) / (*var)
  // +1 == -1 / +1 != -1 / +1 && -1
  for (int i = 0; i < nr_token; i++) {
    if (i == 0 || tokens[i - 1].type == TK_L_Parenthese || tokens[i - 1].type >= TK_EQ) {
      switch (tokens[i].type) {
      case TK_ADD:
        tokens[i].type = TK_POS;
        break;
      case TK_SUB:
        tokens[i].type = TK_NEG;
        break;
      case TK_MUL:
        tokens[i].type = TK_DEREF;
        break;
      default:
        break;
      }
    }
  }

  bad_expression = false;
  word_t result = eval(0, nr_token - 1);
  if (bad_expression) {
    *success = false;
  }
  return result;
}
