/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "mock.h"

#include <assert.h>
#include <unistd.h>

#include "dev.h"
#include "util.h"

enum {
  FD_READ    = 0, /* read end of incoming pipe */
  FD_INJECT  = 1, /* write end of incoming pipe */
  FD_EXTRACT = 2, /* read end of outgoing pipe */
  FD_WRITE   = 3, /* write end of outgoing pipe */
  NUM_FDS    = 4
};

struct mock_priv {
  int fds[NUM_FDS];
  int is_open;
};

static int mock_open(struct dev *dev)
{
  assert(dev);
  struct mock_priv *priv = dev_priv(dev);
  assert(priv);

  return 0;
}

static int mock_read(struct dev *dev, void *buf, size_t len)
{
  assert(dev);
  assert(buf);
  struct mock_priv *priv = dev_priv(dev);
  assert(priv);

  /* TODO: Actually read. */
  len = len;

  return 0;
}

static int mock_write(struct dev *dev, const void *buf, size_t len)
{
  assert(dev);
  assert(buf);
  struct mock_priv *priv = dev_priv(dev);
  assert(priv);
  assert(priv->is_open);

  /* TODO: Actually write. */
  len = len;

  return 0;
}

static int mock_close(struct dev *dev)
{
  assert(dev);
  struct mock_priv *priv = dev_priv(dev);
  assert(priv);
  assert(priv->is_open);

  priv->is_open = 0;

  return 0;
}

static void mock_destroy(struct dev *dev)
{
  assert(dev);
  struct mock_priv *priv = dev_priv(dev);
  xfree(priv);
}

struct dev_ops mock_ops = {
  .open = &mock_open,
  .read = &mock_read,
  .write = &mock_write,
  .close = &mock_close,
  .destroy = &mock_destroy
};

struct dev *mock_new(void)
{
  int result;

  struct mock_priv *priv = xmalloc(sizeof(*priv));
  result = pipe(&priv->fds[0]);
  if (result < 0) {
    xfree(priv);
    return NULL;
  }
  result = pipe(&priv->fds[2]);
  if (result < 0) {
    xfree(priv);
    return NULL;
  }
  struct dev *dev = dev_new(&mock_ops, priv);
  return dev;
}
