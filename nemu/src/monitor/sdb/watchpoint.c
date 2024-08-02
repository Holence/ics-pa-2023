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

static WP wp_pool[NR_WP] = {};
static WP *wp_assigned = NULL;
static WP *wp_free = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  wp_assigned = NULL;
  wp_free = wp_pool;
}

/* Implement the functionality of watchpoint */
WP *new_wp(char *expr_, word_t init_value) {
  if (wp_free == NULL) {
    return NULL;
  }

  // 从wp_free的头上取出
  WP *wp = wp_free;
  wp_free = wp_free->next;
  strcpy(wp->expr_, expr_);
  wp->next = NULL;
  wp->old_value = init_value;

  // 添加在wp_assigned的尾部
  if (wp_assigned == NULL) {
    wp_assigned = wp;
  } else {
    WP *ptr = wp_assigned;
    while (ptr->next != NULL) {
      ptr = ptr->next;
    }
    ptr->next = wp;
  }

  return wp;
}

WP *free_wp(int NO_) {
  if (wp_assigned == NULL) {
    return NULL;
  } else {
    WP *ptr = wp_assigned;
    WP *ptr_prev = NULL;
    WP *wp = NULL;
    while (ptr) {
      if (ptr->NO == NO_) {
        wp = ptr;
        break;
      }
      ptr_prev = ptr;
      ptr = ptr->next;
    }

    if (wp == NULL) {
      return NULL;
    } else {
      // 从wp_assigned中取出
      if (ptr_prev == NULL) {
        // 第一个就是wp
        wp_assigned = wp->next;
      } else {
        // wp在中间
        ptr_prev->next = wp->next;
      }

      // 放到wp_free的头上
      wp->next = wp_free;
      wp_free = wp;

      return wp;
    }
  }
}

void print_wp() {
  WP *ptr = wp_assigned;
  while (ptr != NULL) {
    printf("%d: %s\n", ptr->NO, ptr->expr_);
    ptr = ptr->next;
  }
}