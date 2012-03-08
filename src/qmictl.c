/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmictl: QMI calls on the CTL (control) service.
 *
 * Sources used in writing this file (see README for links):
 * [GobiNet]/QMI.c
 * [GobiNet]/QMIDevice.c
 * [cros-kerne]/drivers/net/usb/gobi/qmi.c
 * [cros-kerne]/drivers/net/usb/gobi/qmidevice.c
 */

#include "qmictl.h"

#include <assert.h>

#include "qmidev.h"
#include "qmimsg.h"
#include "qrb.h"
#include "util.h"

/* This will move to qmi.h once that exists and we add multiple services */
#define QMI_SERVICE_CTL 0x00
/* CTL is always client zero. */
#define QMI_CTL_CLIENT 0x00

enum {
  QMICTL_MSG_GET_CID = 0x0022,
  QMICTL_MSG_RELEASE_CID = 0x0023
};

#define QMI_TLV_ID 0x01
#define QMI_TLV_RESULT 0x02

static struct qmimsg *qmimsg_ctl_new(uint16_t transaction, uint16_t message)
{
  struct qmimsg *msg;

  if (qmimsg_new(0, QMI_SERVICE_CTL, QMI_CTL_CLIENT,
                 0, transaction, message,
                 &msg))
    return NULL;

  return msg;
}

struct qmi_cid {
  uint8_t service;
  uint8_t client;
};

struct get_cid_ctx {
  qmidev *qmidev;

  qmictl_get_cid_response_fn cb;
  void *context;

  uint8_t service;
};

static void get_cid_cb(struct qrb *qrb, struct qmimsg *msg)
{
  struct get_cid_ctx *ctx = qrb_priv(qrb);
  assert(ctx);
  struct qmi_cid id;
  struct { uint16_t status; uint16_t error; } result;

  if (qmimsg_tlv_get(msg, QMI_TLV_RESULT, sizeof(result), &result) ||
      result.status != 0) {
    goto fail;
  }

  if (qmimsg_tlv_get(msg, QMI_TLV_ID, sizeof(id), &id) ||
      id.service != ctx->service) {
    goto fail;
  }

  ctx->cb(ctx->qmidev, ctx->context, 0, id.client);
  goto out;

fail:
  ctx->cb(ctx->qmidev, ctx->context, -1, 0);

out:
  xfree(ctx);
  qrb_set_priv(qrb, NULL);
}

int qmictl_get_cid(struct qmidev *qmidev, uint8_t service,
                   qmictl_get_cid_response_fn callback, void *context)
{
  struct qmimsg *msg;
  struct qrb *qrb;
  struct get_cid_ctx *ctx = xmalloc(sizeof(*ctx));
  uint16_t tid = 0x0001; /* TODO: ACtually allocate a tid. */

  msg = qmimsg_ctl_new(tid, QMICTL_MSG_GET_CID);
  qmimsg_tlv_add(msg, QMI_TLV_ID, sizeof(service), &service);
  qmidev_send(qmidev, msg);

  ctx->qmidev = qmidev;
  ctx->cb = callback;
  ctx->context = context;
  ctx->service = service;

  qrb = qrb_alloc(QMI_SERVICE_CTL, QMI_CTL_CLIENT, tid, 0, get_cid_cb, ctx);
  qmidev_listen(qmidev, qrb);

  return 0;
}

struct release_cid_ctx {
  qmidev *qmidev;

  qmidev_response_fn cb;
  void *context;

  struct qmi_cid id;
};

static void release_cid_cb(struct qrb *qrb, struct qmimsg *msg)
{
  struct release_cid_ctx *ctx = qrb_priv(qrb);
  assert(ctx);
  struct qmi_cid id;
  struct { uint16_t status; uint16_t error; } result;

  if (qmimsg_tlv_get(msg, QMI_TLV_RESULT, sizeof(result), &result) ||
      result.status != 0) {
    goto fail;
  }

  if (qmimsg_tlv_get(msg, QMI_TLV_ID, sizeof(id), &id) ||
      id.service != ctx->id.service ||
      id.client != ctx->id.client) {
    goto fail;
  }

  ctx->cb(ctx->qmidev, ctx->context, 0);
goto out;

fail:
  ctx->cb(ctx->qmidev, ctx->context, -1);

out:
  xfree(ctx);
  qrb_set_priv(qrb, NULL);
}

int qmictl_release_cid(struct qmidev *qmidev, uint8_t service, uint8_t client,
                       qmidev_response_fn callback, void *context)
{
  struct qmimsg *msg;
  struct qrb *qrb;
  struct release_cid_ctx *ctx = xmalloc(sizeof(*ctx));
  struct qmi_cid id;
  uint16_t tid = 0x0001; /* TODO: Allocate a transaction ID. */

  msg = qmimsg_ctl_new(tid, QMICTL_MSG_RELEASE_CID);
  id.service = service;
  id.client = client;
  qmimsg_tlv_add(msg, QMI_TLV_ID, sizeof(id), &id);
  qmidev_send(qmidev, msg);

  ctx->qmidev = qmidev;
  ctx->cb = callback;
  ctx->context = context;
  ctx->id.service = service;
  ctx->id.client = client;

  qrb = qrb_alloc(QMI_SERVICE_CTL, QMI_CTL_CLIENT, tid, 0, release_cid_cb, ctx);
  qmidev_listen(qmidev, qrb);

  return 0;
}
