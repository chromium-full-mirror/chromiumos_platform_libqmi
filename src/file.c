/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "file.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>

#include "dev.h"
#include "qmidev.h"
#include "util.h"

struct file_priv {
  struct qmidev *qmidev;
  char *path;
  int fd;
};

#define FD_CLOSED (-1)

#define FILE_OPEN(priv) ((priv)->fd != FD_CLOSED)

static int file_open(struct dev *dev)
{
  struct file_priv *priv;
  int fd, result;

  assert(dev);
  priv = dev_priv(dev);
  assert(!FILE_OPEN(priv));

  fd = open(priv->path, O_RDWR);
  if (fd < 0)
    return errno;

  result = qmidev_set_dev_fd(priv->qmidev, fd);
  if (result) {
    close(fd);
    return result;
  }

  priv->fd = fd;

  return 0;
}

static int file_read(struct dev *dev, void *buf, size_t len)
{
  struct file_priv *priv;
  ssize_t result;

  assert(dev);
  assert(buf);
  priv = dev_priv(dev);
  assert(FILE_OPEN(priv));

  result = read(priv->fd, buf, len);
  if (result < 0)
    return result;

  /* TODO: short read? */

  return 0;
}

static int file_write(struct dev *dev, const void *buf, size_t len)
{
  struct file_priv *priv;
  ssize_t result;

  assert(dev);
  assert(buf);
  priv = dev_priv(dev);
  assert(FILE_OPEN(priv));

  result = write(priv->fd, buf, len);
  if (result < 0)
    return result;

  /* TODO: short write? */

  return 0;
}

static int file_close(struct dev *dev)
{
  struct file_priv *priv;
  int result;

  assert(dev);
  priv = dev_priv(dev);
  assert(FILE_OPEN(priv));

  result = close(priv->fd);
  if (result < 0)
    return result;

  priv->fd = FD_CLOSED;

  qmidev_clear_dev_fd(priv->qmidev);

  return 0;
}

static void file_destroy(struct dev *dev)
{
  struct file_priv *priv;

  assert(dev);
  priv = dev_priv(dev);
  assert(!FILE_OPEN(priv));
  g_free(priv->path);
  g_slice_free(struct file_priv, priv);
}

struct dev_ops file_ops = {
  .open = &file_open,
  .read = &file_read,
  .write = &file_write,
  .close = &file_close,
  .destroy = &file_destroy
};

struct dev *file_new(struct qmidev *qmidev, const char *path)
{
  struct file_priv *priv = g_slice_new(struct file_priv);
  priv->qmidev = qmidev;
  priv->path = g_strdup(path);
  priv->fd = FD_CLOSED;
  return dev_new(&file_ops, priv);
}
