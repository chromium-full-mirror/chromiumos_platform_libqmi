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
#include <errno.h>
#include <glib.h>

#include "qmi.h"
#include "qmidev.h"
#include "qmimsg.h"
#include "util.h"

/* CTL is always client zero. */
#define QMI_CTL_CLIENT 0x00

enum {
  QMICTL_MSG_GET_CID = 0x0022,
  QMICTL_MSG_RELEASE_CID = 0x0023
};

#define QMI_TLV_ID ((uint8_t)0x01)

struct qmi_cid {
  uint8_t service;
  uint8_t client;
};

static int ctl_get_id(struct qmimsg *message, struct qmi_cid *id_out)
{
  assert(message);
  assert(id_out);

  return qmimsg_tlv_get(message, QMI_TLV_ID, sizeof(*id_out), id_out);
}

static struct qmimsg *ctl_msg_new(uint16_t transaction, uint16_t message)
{
  struct qmimsg *msg;

  if (qmimsg_new(0, QMI_SVC_CTL, QMI_CTL_CLIENT,
                 0, transaction, message,
                 &msg))
    return NULL;

  return msg;
}

static void ctl_criteria(uint16_t transaction,
                         struct listen_criteria *criteria)
{
  criteria->service = QMI_SVC_CTL;
  criteria->client = QMI_CTL_CLIENT;
  criteria->transaction = transaction;
}

struct get_cid_context {
  qmictl_get_cid_response_fn callback;
  void *context;
  uint8_t service;
};

static int got_cid(struct qmidev *qmidev, void *data,
                   struct qmimsg *message)
{
  assert(qmidev);
  assert(data);
  assert(message);

  struct get_cid_context *context = data;
  struct qmi_cid id;
  uint16_t status; uint16_t error;
  int result;

  id.client = 0;

  result = qmi_result(message, &status, &error);
  if (result)
    goto out;
  if (status) {
    result = status;
    goto out;
  }

  result = ctl_get_id(message, &id);
  if (result)
    goto out;
  if (id.service != context->service) {
    result = EINVAL;
    goto out;
  }

  result = 0;

out:
  context->callback(qmidev, context->context, result, id.client);
  g_slice_free(struct get_cid_context, context);
  return 1;
}

int qmictl_get_cid(struct qmidev *qmidev, uint8_t service,
                   qmictl_get_cid_response_fn caller_callback,
                   void *caller_context)
{
  struct get_cid_context *context = g_slice_new(struct get_cid_context);
  context->callback = caller_callback;
  context->context  = caller_context;
  context->service  = service;

  uint16_t tid = 0x0001; /* TODO: ACtually allocate a tid. */

  struct qmimsg *message;
  message = ctl_msg_new(tid, QMICTL_MSG_GET_CID);
  qmimsg_tlv_add(message, QMI_TLV_ID, sizeof(service), &service);
  qmidev_send(qmidev, message);

  struct listen_criteria criteria;
  ctl_criteria(tid, &criteria);
  qmidev_listen(qmidev, &criteria, &got_cid, context);

  return 0;
}

struct release_cid_context {
  qmidev_response_fn callback;
  void *context;
  struct qmi_cid id;
};

static int released_cid(struct qmidev *qmidev, void *data,
                        struct qmimsg *message)
{
  assert(qmidev);
  assert(data);
  assert(message);

  struct release_cid_context *context = data;
  struct qmi_cid id;
  uint16_t status; uint16_t error;
  int result;

  result = qmi_result(message, &status, &error);
  if (result)
    goto out;
  if (status) {
    result = error;
    goto out;
  }

  result = ctl_get_id(message, &id);
  if (result)
    goto out;
  if (id.service != context->id.service ||
      id.client != context->id.client) {
    result = EINVAL;
    goto out;
  }

  result = 0;

out:
  context->callback(qmidev, context->context, result);
  g_slice_free(struct release_cid_context, context);
  return 1;
}

int qmictl_release_cid(struct qmidev *qmidev, uint8_t service, uint8_t client,
                       qmidev_response_fn caller_callback,
                       void *caller_context)
{
  struct release_cid_context *context =
      g_slice_new(struct release_cid_context);
  context->callback   = caller_callback;
  context->context    = caller_context;
  context->id.service = service;
  context->id.client  = client;

  uint16_t tid = 0x0001; /* TODO: Allocate a transaction ID. */
  struct qmimsg *message = ctl_msg_new(tid, QMICTL_MSG_RELEASE_CID);
  qmimsg_tlv_add(message, QMI_TLV_ID, sizeof(context->id), &context->id);
  qmidev_send(qmidev, message);

  struct listen_criteria criteria;
  ctl_criteria(tid, &criteria);
  qmidev_listen(qmidev, &criteria, &released_cid, context);

  return 0;
}
