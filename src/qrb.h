/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qrb: QMI Request Block, or how you listen for incoming QMI messages.
 */

#ifndef LIBQMI_QRB_H
#define LIBQMI_QRB_H

#include <stdint.h>

struct qrb;
struct qmimsg;

/**
 * Callback type for qrbs.
 */
typedef void (*qrb_callback)(struct qrb *qrb, struct qmimsg *msg);

enum {
  /**
   * A "persistent" QRB is not removed from a qrbset once it completes
   * or times out.  (An example of a QRB that should be persistent is
   * one that listens for unsolicited callbacks from the device.)
   */
  QRB_PERSISTENT = 0x01
};

/*
 * TODO:
 * Separate priv/ctx for callback?  (Will other parts of my code use priv?)
 * In-place init/destroy of qrb/qrbset?  (Do we want/need this?)
 * New lifecycle.
 */

/**
 * Allocates a new QRB.
 *
 * @svc: The QMI service of messages this QRB will listen for
 * @cid: The client ID of messages this QRB will listen for
 * @tid: The transaction ID of messages this QRB will listen for
 * @flags: The bitwise-OR of QRB_* flags that apply to this QRB
 * @callback: The callback function to call when a message is received
 * @priv: A pointer to stash in the QRB (can be retrieved with qrb_priv)
 */
struct qrb *qrb_alloc(uint8_t svc, uint8_t cid, uint16_t tid, int flags,
                      qrb_callback callback, void *priv);

/**
 * Frees a QRB allocated by qrb_alloc.
 */
void qrb_free(struct qrb *qrb);

/**
 * Gets the private pointer stored in this QRB.
 */
void *qrb_priv(struct qrb *qrb);

/**
 * Sets the private pointer stored in this QRB.
 */
void qrb_set_priv(struct qrb *qrb, void *priv);

/**
 * Allocates a new qrbset.
 */
struct qrbset *qrbset_alloc(void);

/**
 * Adds a QRB to this qrbset.
 */
void qrbset_add(struct qrbset *set, struct qrb *qrb);

/**
 * Removes a QRB from this qrbset.
 */
void qrbset_remove(struct qrbset *set, struct qrb *qrb);

/**
 * Finds and completes any QRBs in this qrbset that match the given QMI
 * message.
 */
int qrbset_complete(struct qrbset *set, struct qmimsg *msg);

/**
 * Frees a qrbset allocated by qrbset_alloc.
 */
void qrbset_free(struct qrbset *set);

#endif /* LIBQMI_QRB_H */
