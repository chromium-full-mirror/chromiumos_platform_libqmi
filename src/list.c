/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "list.h"

#include <assert.h>

#include "util.h"

void list_init(struct list *list)
{
  list->head = NULL;
  list->tail = NULL;
}

void list_destroy(struct list *list)
{
  assert(list);
  assert(!list->head);
}

void list_add(struct list *list, struct list_node *node, void *item)
{
  assert(list);

  node->prev = list->tail;
  node->next = NULL;
  node->item = item;

  if (list->tail)
    list->tail->next = node;
  else
    list->head = node;
  list->tail = node;
}

void *list_remove(struct list *list, struct list_node *node)
{
  assert(list);
  assert(node);

  void *item;

  if (node->prev)
    node->prev->next = node->next;
  else
    list->head = node->next;
  if (node->next)
    node->next->prev = node->prev;
  else
    list->tail = node->prev;

  item = node->item;

  return item;
}

struct list_node *list_get_head(struct list *list)
{
  assert(list);

  return list->head;
}

void list_apply(struct list *list, list_apply_fn func, void *context)
{
  assert(list);
  assert(func);

  struct list_node *node, *next;

  if (!list->head)
    return;

  node = list->head;
  while (node) {
    next = node->next;
    if (func(context, node, node->item))
      return;
    node = next;
  }
}
