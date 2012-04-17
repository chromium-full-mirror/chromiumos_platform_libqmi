/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dev.h"

#include <assert.h>
#include <glib.h>

#include "util.h"

enum {
  FLAG_OPEN = 1
};

struct dev {
  struct dev_ops *ops;
  void *priv;
  int flags;
};

static int dev_is_open(struct dev *dev)
{
  return dev->flags & FLAG_OPEN;
}

struct dev *dev_new(struct dev_ops *ops, void *priv)
{
  struct dev *dev;

  assert(ops);

  dev = g_slice_new(struct dev);
  dev->ops = ops;
  dev->priv = priv;
  dev->flags = 0;

  return dev;
}

void dev_destroy(struct dev *dev)
{
  assert(dev);
  assert(!dev_is_open(dev));
  assert(dev->ops->destroy);

  dev->ops->destroy(dev);

  g_slice_free(struct dev, dev);
}

void *dev_priv(struct dev *dev)
{
  assert(dev);
  return dev->priv;
}

int dev_open(struct dev *dev)
{
  int result;

  assert(dev);
  assert(!dev_is_open(dev));
  assert(dev->ops->open);

  result = dev->ops->open(dev);

  if (result == 0)
    dev->flags |= FLAG_OPEN;

  return result;
}

int dev_read(struct dev *dev, void *buf, size_t len)
{
  assert(dev);
  assert(dev_is_open(dev));
  assert(dev->ops->read);

  return dev->ops->read(dev, buf, len);
}

int dev_write(struct dev *dev, const void *buf, size_t len)
{
  assert(dev);
  assert(dev_is_open(dev));
  assert(dev->ops->write);

  return dev->ops->write(dev, buf, len);
}

int dev_close(struct dev *dev)
{
  int result;

  assert(dev);
  assert(dev_is_open(dev));
  assert(dev->ops->close);

  result = dev->ops->close(dev);

  if (result == 0)
    dev->flags &= ~FLAG_OPEN;

  return result;
}
