/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * dev: abstraction for a character device (or mock, proxy, &c).
 *
 * A dev is the raw character device that underlies a qmidev.
 * The qmidev reads and writes QMI messages from and to the dev, and can
 * open and close it at will.
 * Right now, there are two implementations of dev: file (for an actual file)
 * and mock (for testing purposes).
 */

#ifndef LIBQMI_DEV_H
#define LIBQMI_DEV_H

#include <stddef.h>

struct dev;

/* TODO: Pass dev->priv directly to callbacks, and get rid of dev_priv? */

/**
 * Operations that can be performed on a device.
 *
 * All of these ops return zero on success, non-zero on error.
 *
 * All of these ops except destroy can block; it is expected that read will
 * only be called when the qmidev knows the device has data to read, and the
 * other calls will be "fast enough".
 */
struct dev_ops {
  /**
   * Called to open dev.
   *
   * open will only be called while dev is closed.
   * If open succeeds, dev is now considered open.
   */
  int (*open)(struct dev *dev);

  /**
   * Called to read len bytes from dev into buf.
   *
   * This read operation cannot succeed partially; if backed by one that
   * can, it should retry until the entire buffer has been filled, or
   * return an error if it will never be filled.
   *
   * read will only be called while dev is open.
   */
  int (*read)(struct dev *dev, void *buf, size_t len);

  /**
   * Called to write len bytes from buf to dev.
   *
   * This write operation cannot succeed partially; if backed by one
   * that can, it shold retry until the entire buffer has been written,
   * or return an error if it will never be written.
   *
   * write will only be called while dev is open.
   */
  int (*write)(struct dev *dev, const void *buf, size_t len);

  /**
   * Called to close dev.
   *
   * close will only be called while dev is open.
   * If close succeeds, dev is now considered closed.
   */
  int (*close)(struct dev *dev);

  /**
   * Called to destroy dev.
   *
   * destroy will only be called while dev is closed.
   * destroy must succeed; once it does, the device layer will no longer
   * call any callbacks on dev.
   */
  void (*destroy)(struct dev *dev);
};

/* Allocates a new device object with a given set of ops and priv pointer. */
struct dev *dev_new(struct dev_ops *ops, void *priv);

/* Destroys a device object (calls ops->destroy and frees the device). */
void dev_destroy(struct dev *dev);

/**
 * Gets the priv pointer of the device object.
 * (Intended for use by dev_ops callbacks.)
 */
void *dev_priv(struct dev *dev);

/* Opens dev. */
int dev_open(struct dev *dev);

/* Reads len bytes from dev into buf. */
int dev_read(struct dev *dev, void *buf, size_t len);

/* Writes len bytes from buf to dev. */
int dev_write(struct dev *dev, const void *buf, size_t len);

/* Closes dev. */
int dev_close(struct dev *dev);

#endif /* LIBQMI_DEV_H */
