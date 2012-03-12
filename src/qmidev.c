/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "qmidev.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <libqmi.h>

#include "dev.h"
#include "file.h"
#include "list.h"
#include "mock.h"
#include "poller.h"
#include "qmimsg.h"
#include "qrb.h"
#include "util.h"

struct qmidev {
  void *priv;

  struct poller *poller;

  struct dev *dev;
  struct polled *dev_polled;
  int timerfd;
  struct polled *timer_polled;

  struct qrbset *qrbs;
  struct list callbacks;

  int connected;
  uint8_t control_tid;
};

static void qmidev_call_callbacks(struct qmidev *qmidev);

static struct qmidev *qmidev_new(void)
{
  struct qmidev *qmidev = xmalloc(sizeof(*qmidev));

  qmidev->priv = NULL;

  qmidev->poller = poller_new();
  qmidev->dev = NULL;
  qmidev->dev_polled = NULL;

  qmidev->qrbs = qrbset_alloc();
  list_init(&qmidev->callbacks);

  qmidev->connected = 0;
  qmidev->control_tid = 1;

  return qmidev;
}

struct qmidev *qmidev_new_file(const char *path)
{
  assert(path);

  struct qmidev *qmidev = qmidev_new();

  struct dev *dev = file_new(qmidev, path);
  if (!dev) {
    free(qmidev);
    return NULL;
  }
  qmidev->dev = dev;

  return qmidev;
}

struct qmidev *qmidev_new_mock(void)
{
  struct qmidev *qmidev = qmidev_new();

  struct dev *dev = mock_new();
  qmidev->dev = dev;

  return qmidev;
}

void *qmidev_priv(struct qmidev *qmidev)
{
  assert(qmidev);
  return qmidev->priv;
}

void qmidev_set_priv(struct qmidev *qmidev, void *priv)
{
  assert(qmidev);
  qmidev->priv = priv;
}

int qmidev_fd(struct qmidev *qmidev)
{
  assert(qmidev);
  return poller_fd(qmidev->poller);
}

int qmidev_process(struct qmidev *qmidev)
{
  assert(qmidev);
  qmidev_call_callbacks(qmidev);
  return poller_poll(qmidev->poller);
}

struct connect_context {
  qmidev_response_fn callback;
  void *context;
  int status;
};

/* TODO: This can be made generic, for any qmidev_response_fn. */
static void connect_done(struct qmidev *qmidev, void *data)
{
  struct connect_context *context = data;

  context->callback(qmidev, context->context, context->status);
  xfree(context);
}

int qmidev_connect(struct qmidev *qmidev, qmidev_response_fn cb, void *ctx)
{
  assert(qmidev);
  assert(cb);
  assert(!qmidev->connected);

  int result = dev_open(qmidev->dev);
  if (result < 0)
    return result;

  qmidev->connected = 1;

  /* TODO: Make sure we can talk to card, allocate clients, etc. */

  struct connect_context *context = xmalloc(sizeof(*context));
  context->callback = cb;
  context->context = ctx;
  context->status = 0;
  qmidev_call_later(qmidev, &connect_done, context);

  return 0;
}

static void disconnect_done(struct qmidev *qmidev, void *data)
{
  struct connect_context *context = data;

  context->callback(qmidev, context->context, context->status);
  xfree(context);
}

int qmidev_disconnect(struct qmidev *qmidev, qmidev_response_fn cb, void *ctx)
{
  assert(qmidev);
  assert(cb);
  assert(qmidev->connected);

  /* TODO: Release clients, etc. */

  int result = dev_close(qmidev->dev);
  if (result < 0)
    return result;

  qmidev->connected = 0;

  struct connect_context *context = xmalloc(sizeof(*context));
  context->callback = cb;
  context->context = ctx;
  context->status = 0;
  qmidev_call_later(qmidev, &disconnect_done, context);

  return 0;
}

static int dev_read_fn(void *ctx, void *buf, size_t len)
{
  assert(ctx);
  struct qmidev *qmidev = ctx;
  assert(qmidev->dev);

  return dev_read(qmidev->dev, buf, len);
}

static void dev_read_cb(struct polled *polled)
{
  assert(polled);
  struct qmidev *qmidev = polled_priv(polled);
  assert(qmidev);

  struct qmimsg *msg;
  int result = qmimsg_read(&dev_read_fn, qmidev, &msg);
  /* TODO: Do something. */
  if (result)
    return;
  qrbset_complete(qmidev->qrbs, msg);
  qmimsg_free(msg);
}

static void dev_close_cb(struct polled *polled)
{
  assert(polled);
  struct qmidev *qmidev = polled_priv(polled);
  assert(qmidev);

  qmidev_clear_dev_fd(qmidev);
}

int qmidev_set_dev_fd(struct qmidev *qmidev, int fd)
{
  assert(qmidev);
  assert(fd >= 0);
  assert(!qmidev->dev_polled);

  qmidev->dev_polled = poller_add(qmidev->poller, fd);
  if (!qmidev->dev_polled)
    return ENOMEM;
  polled_set_priv(qmidev->dev_polled, qmidev);
  polled_set_read(qmidev->dev_polled, &dev_read_cb);
  polled_set_close(qmidev->dev_polled, &dev_close_cb);
  polled_update(qmidev->dev_polled);

  return 0;
}

int qmidev_clear_dev_fd(struct qmidev *qmidev)
{
  assert(qmidev);
  assert(qmidev->dev_polled);

  /* Make sure we don't get called again. */
  polled_set_close(qmidev->dev_polled, NULL);
  polled_update(qmidev->dev_polled);

  /* will be freed by the poller when the fd closes. */
  qmidev->dev_polled = NULL;

  return 0;
}

static int dev_write_fn(void *ctx, const void *buf, size_t len)
{
  assert(ctx);
  struct qmidev *qmidev = ctx;
  assert(qmidev->dev);

  return dev_write(qmidev->dev, buf, len);
}

int qmidev_send(struct qmidev *qmidev, struct qmimsg *msg)
{
  assert(qmidev);
  assert(qmidev->connected);

  return qmimsg_write(msg, &dev_write_fn, qmidev);
}

struct polled *qmidev_poll(struct qmidev *qmidev, int fd)
{
  assert(qmidev);

  return poller_add(qmidev->poller, fd);
}

int qmidev_listen(struct qmidev *qmidev, struct qrb *qrb)
{
  assert(qmidev);
  assert(qrb);

  qrbset_add(qmidev->qrbs, qrb);

  return 0;
}

struct callback {
  callback_fn func;
  void *context;
  struct list_node node;
};

void qmidev_call_later(struct qmidev *qmidev, callback_fn func, void *context)
{
  assert(qmidev);
  assert(func);

  struct callback *callback = xmalloc(sizeof(*callback));
  callback->func = func;
  callback->context = context;
  list_add(&qmidev->callbacks, &callback->node, callback);
}

static int call_callback(void *context, struct list_node *node, void *item)
{
  struct qmidev *qmidev = context;
  struct callback *callback = item;

  callback->func(qmidev, callback->context);
  list_remove(&qmidev->callbacks, node);
  xfree(callback);

  return 0;
}

void qmidev_call_callbacks(struct qmidev *qmidev)
{
  list_apply(&qmidev->callbacks, call_callback, qmidev);
}
