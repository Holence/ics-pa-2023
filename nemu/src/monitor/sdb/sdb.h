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

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* Add more members if necessary */
  char expr_[32];
  word_t old_value;
} WP;

#define NR_WP 32

word_t expr(char *e, bool *success);
WP *new_wp(char *expr_, word_t init_value);
WP *free_wp(int NO_);
void print_wp();
void wp_check_changed();

#endif
