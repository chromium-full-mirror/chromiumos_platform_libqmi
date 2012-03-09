/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * list: Generic linked list
 */

#ifndef LIBQMI_LIST_H
#define LIBQMI_LIST_H

struct list_node {
  struct list_node *prev;
  struct list_node *next;
  void *item;
};

struct list {
  struct list_node *head;
  struct list_node *tail;
};

/**
 * Callback type for list_apply.  If it returns a non-zero value, list_apply
 * returns immediately, ignoring the rest of the list.
 * @context: the context passed to list_apply
 * @node: the current list node
 * @item: the item contained in node
 */
typedef int (*list_apply_fn)(void *context,
                             struct list_node *node, void *item);

/**
 * Initializes a linked list (in place; does not allocate it).
 * @list: The list to initialize
 */
void list_init(struct list *list);

/**
 * Destroys a linked list (in place; does not free it).
 * @list: The list to destroy
 */
void list_destroy(struct list *list);

/**
 * Adds an item to a linked list (at the end).
 *
 * Does not allocate a struct list_node; the intent is that the caller will
 * embed one in the item struct, to save the extra allocation.
 *
 * @list: The list to add the item to
 * @node: An allocated struct list_node for the list to use
 * @item: The item to add
 */
void list_add(struct list *list, struct list_node *node, void *item);

/**
 * Removes an node from a linked list and returns the removed item.
 *
 * Does not free the struct list_node.
 *
 * @list: The list to remove the item from
 * @node: The node of the item to remove
 */
void *list_remove(struct list *list, struct list_node *node);

/**
 * Returns the first node in a linked list.
 *
 * @list: The list to get the head of
 */
struct list_node *list_get_head(struct list *list);

/**
 * Applies a function to each node in a linked list.
 *
 * func can stop iteration by returning a non-zero value.
 *
 * It is safe for func to remove the node that it has been called with, but
 * any other modifications to the list are prohibited.
 *
 * @list: the list
 * @func: the function
 * @context: a context to pass to func, along with the node and item
 */
void list_apply(struct list *list, list_apply_fn func, void *context);

#endif /* LIBQMI_LIST_H */
