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

struct get_cid_context {
  qmictl_get_cid_response_fn callback;
  void *context;
  uint8_t service;
};

static void got_cid(struct qmidev *qmidev,
                   void *data,
                   int status,
                   struct qmimsg *message)
{
  assert(qmidev);
  assert(data);
  assert(message);

  struct get_cid_context *context = data;
  struct qmi_cid id;
  int result;

  id.client = 0;

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
}

int qmictl_get_cid(struct qmidev *qmidev, uint8_t service,
                   qmictl_get_cid_response_fn caller_callback,
                   void *caller_context)
{
  int result;

  struct get_cid_context *context = g_slice_new(struct get_cid_context);
  context->callback = caller_callback;
  context->context  = caller_context;
  context->service  = service;

  struct qmimsg *message;
  result = qmidev_make_request(qmidev, QMI_SVC_CTL, QMICTL_MSG_GET_CID,
                               &message);
  if (result)
    goto fail;

  qmimsg_tlv_add(message, QMI_TLV_ID, sizeof(service), &service);

  result = qmidev_send_request(qmidev, message,
                               QMICTL_MSG_GET_CID,
                               &got_cid, context);
  if (result)
    goto fail;

  return 0;

fail:
  g_slice_free(struct get_cid_context, context);
  return result;
}

struct release_cid_context {
  qmidev_response_fn callback;
  void *context;
  struct qmi_cid id;
};

static void released_cid(struct qmidev *qmidev,
                         void *data,
                         int status,
                         struct qmimsg *message)
{
  assert(qmidev);
  assert(data);
  assert(message);

  struct release_cid_context *context = data;
  struct qmi_cid id;
  int result;

  if (status) {
    result = status;
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
}

int qmictl_release_cid(struct qmidev *qmidev, uint8_t service, uint8_t client,
                       qmidev_response_fn caller_callback,
                       void *caller_context)
{
  int result;

  struct release_cid_context *context =
      g_slice_new(struct release_cid_context);
  context->callback   = caller_callback;
  context->context    = caller_context;
  context->id.service = service;
  context->id.client  = client;

  struct qmimsg *message;
  result = qmidev_make_request(qmidev, QMI_SVC_CTL, QMICTL_MSG_RELEASE_CID,
                               &message);
  if (result)
    goto fail;

  qmimsg_tlv_add(message, QMI_TLV_ID, sizeof(context->id), &context->id);

  result = qmidev_send_request(qmidev, message,
                               QMICTL_MSG_RELEASE_CID,
                               &released_cid, context);
  if (result)
    goto fail;

  return 0;

fail:
  g_slice_free(struct release_cid_context, context);
  return result;
}
