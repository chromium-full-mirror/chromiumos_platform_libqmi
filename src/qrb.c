/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "qrb.h"

#include <assert.h>
#include <stdio.h>

#include "qmidev.h"
#include "qmimsg.h"
#include "util.h"

/* TODO: Need to remove/free qrbs while in qrbset_complete. */

/* TODO: Do we need to refcount these? */
struct qrb {
  int flags;

  uint8_t service;
  uint8_t client;
  uint16_t transaction;

  qrb_callback callback;
  void *priv;

  struct qrb *next;
  struct qrb *prev;
};

struct qrb *qrb_alloc(uint8_t s, uint8_t c, uint16_t t, int flags,
          qrb_callback cb, void *priv)
{
  struct qrb *qrb = xmalloc(sizeof(*qrb));

  qrb->flags = flags;

  qrb->service = s;
  qrb->client = c;
  qrb->transaction = t;

  qrb->callback = cb;
  qrb->priv = priv;

  qrb->next = NULL;
  qrb->prev = NULL;

  return qrb;
}

void qrb_free(struct qrb *qrb)
{
  assert(qrb);
  /* close(qrb->timerfd); */
  xfree(qrb);
}

void *qrb_priv(struct qrb *qrb)
{
  assert(qrb);
  return qrb->priv;
}

void qrb_set_priv(struct qrb *qrb, void *priv)
{
  assert(qrb);
  qrb->priv = priv;
}

static void qrb_complete(struct qrb *qrb, struct qmimsg *msg)
{
  assert(qrb);
  (qrb->callback)(qrb, msg);
}

static int qrb_match(struct qrb *qrb, struct qmimsg *msg)
{
  assert(qrb);
  assert(msg);

  uint8_t service;
  uint8_t client;
  uint16_t transaction;

  qmimsg_get_header(msg, &service, &client, &transaction);

  if (service != qrb->service)
    return 0;

  if (client != qrb->client && client != QMI_CLIENT_BROADCAST)
    return 0;

  /* TODO: Sometimes we don't have a tid. */
  if (transaction != qrb->transaction)
    return 0;

  return 1;
}

struct qrbset {
  struct qrb *head;
};

struct qrbset *qrbset_alloc(void)
{
  struct qrbset *set = xmalloc(sizeof(*set));
  set->head = NULL;
  return set;
}

void qrbset_add(struct qrbset *set, struct qrb *qrb)
{
  assert(set);
  assert(qrb);

  if (set->head)
    set->head->prev = qrb;

  qrb->prev = NULL;
  qrb->next = set->head;

  set->head = qrb;
}

void qrbset_remove(struct qrbset *set, struct qrb *qrb)
{
  assert(set);
  assert(qrb);

  if (set->head == qrb)
    set->head = qrb->next;
  if (qrb->prev)
    qrb->prev->next = qrb->next;
  if (qrb->next)
    qrb->next->prev = qrb->prev;
}

int qrbset_complete(struct qrbset *set, struct qmimsg *msg)
{
  assert(set);
  assert(msg);

  struct qrb *qrb, *next;
  int found = 0;
  if (!set->head)
    goto out;
  for (qrb = set->head, next = qrb->next;
       qrb;
       qrb = next) {
    next = qrb->next;
    if (qrb_match(qrb, msg)) {
      qrb_complete(qrb, msg);
      found = 1;
      if (!(qrb->flags & QRB_PERSISTENT)) {
        qrbset_remove(set, qrb);
      }
    }
  }

out:
  return found;
}

void qrbset_free(struct qrbset *set)
{
  assert(set);
  /* TODO: unlink qrbs? */
  assert(!set->head);
  xfree(set);
}
