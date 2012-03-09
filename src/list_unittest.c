/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>

#include "list.h"

static char *foo = "foo", *bar = "bar", *baz = "baz";
static struct list_node foo_node, bar_node, baz_node;
static int seen_foo, seen_bar, seen_baz;

static char *a_useless_context = "context";

int callback(void *context, struct list_node *node, void *item)
{
  assert(context == a_useless_context);

  if (node == &foo_node) {
    assert(item == foo);
    seen_foo++;
  } else if (node == &bar_node) {
    assert(item == bar);
    seen_bar++;
  } else if (node == &baz_node) {
    assert(item == baz);
    seen_baz++;
  } else {
    assert(0);
  }

  return 0;
}

int callback_stop(void *context, struct list_node *node, void *item)
{
  callback(context, node, item);

  return node == &bar_node;
}

void test_apply(struct list *list, int exp_foo, int exp_bar, int exp_baz)
{
  seen_foo = seen_bar = seen_baz = 0;
  list_apply(list, callback, a_useless_context);
  assert(seen_foo == exp_foo);
  assert(seen_bar == exp_bar);
  assert(seen_baz == exp_baz);
}

void test_apply_stop(struct list *list, int exp_foo, int exp_bar, int exp_baz)
{
  seen_foo = seen_bar = seen_baz = 0;
  list_apply(list, callback_stop, a_useless_context);
  assert(seen_foo == exp_foo);
  assert(seen_bar == exp_bar);
  assert(seen_baz == exp_baz);
}

int main(void)
{
  struct list list;

  list_init(&list);
  assert(!list_get_head(&list));
  test_apply(&list, 0, 0, 0);

  list_add(&list, &foo_node, foo);
  assert(list_get_head(&list) == &foo_node);
  test_apply(&list, 1, 0, 0);
  list_add(&list, &bar_node, bar);
  assert(list_get_head(&list) == &foo_node);
  test_apply(&list, 1, 1, 0);
  list_add(&list, &baz_node, baz);
  assert(list_get_head(&list) == &foo_node);
  test_apply(&list, 1, 1, 1);

  test_apply_stop(&list, 1, 1, 0);

  assert(list_remove(&list, &bar_node) == bar);
  assert(list_get_head(&list) == &foo_node);
  test_apply(&list, 1, 0, 1);
  assert(list_remove(&list, &foo_node) == foo);
  assert(list_get_head(&list) == &baz_node);
  test_apply(&list, 0, 0, 1);
  assert(list_remove(&list, &baz_node) == baz);
  assert(!list_get_head(&list));
  test_apply(&list, 0, 0, 0);

  list_destroy(&list);

  return 0;
}
