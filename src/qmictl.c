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

#include "error.h"
#include "qmi.h"
#include "qmidev.h"
#include "qmimsg.h"
#include "util.h"

struct service qmictl_service = { QMI_SVC_CTL, QMICTL_MSG_EVENT, NULL, NULL };

struct qmi_cid {
  uint8_t service;
  uint8_t client;
};

static int ctl_get_id(struct qmimsg *message, struct qmi_cid *id_out)
{
  assert(message);
  assert(id_out);

  return qmimsg_tlv_get(message, QMI_TLV_VALUE, sizeof(*id_out), id_out);
}

/* QMI CTL call: Get Client ID */

struct get_cid_context {
  qmictl_get_cid_response_fn callback;
  void *context;
  uint8_t service;
};

static int get_cid_request(void *data,
                           struct qmimsg *message)
{
  uint8_t *service = data;
  return qmimsg_tlv_add(message, QMI_TLV_VALUE, sizeof(*service), service);
}

static void get_cid_response(struct qmidev *qmidev,
                             void *data,
                             int result,
                             struct qmimsg *message)
{
  struct get_cid_context *context = data;
  struct qmi_cid id = { 0, 0 };

  if (result)
    goto out;

  result = ctl_get_id(message, &id);
  if (result)
    goto out;

  if (id.service != context->service) {
    result = QMI_ERR_MESSAGE_INVALID;
    goto out;
  }

out:
  context->callback(qmidev, context->context, result, id.client);
  g_slice_free(struct get_cid_context, context);
}

int qmictl_get_cid(struct qmidev *qmidev, uint8_t service,
                   qmictl_get_cid_response_fn caller_callback,
                   void *caller_context)
{
  struct get_cid_context *context;
  int result;

  context = g_slice_new(struct get_cid_context);
  context->callback = caller_callback;
  context->context  = caller_context;
  context->service  = service;

  result = qmidev_request(qmidev, &qmictl_service,
                          QMICTL_MSG_GET_CLIENT_ID,
                          &get_cid_request, &service,
                          &get_cid_response, context);
  if (result)
    g_slice_free(struct get_cid_context, context);
  return result;
}

/* QMI CTL call: Release Client ID */

struct release_cid_context {
  qmidev_response_fn callback;
  void *context;
  struct qmi_cid id;
};

static int release_cid_request(void *data,
                               struct qmimsg *message)
{
  struct qmi_cid *id = data;
  return qmimsg_tlv_add(message, QMI_TLV_VALUE, sizeof(*id), id);
}

static void release_cid_response(struct qmidev *qmidev,
                                 void *data,
                                 int result,
                                 struct qmimsg *message)
{
  struct release_cid_context *context;
  struct qmi_cid id;

  assert(qmidev);
  assert(data);
  assert(message);

  context = data;

  if (result)
    goto out;

  result = ctl_get_id(message, &id);
  if (result)
    goto out;

  if (id.service != context->id.service ||
      id.client != context->id.client)
    result = QMI_ERR_MESSAGE_INVALID;

out:
  context->callback(qmidev, context->context, result);
  g_slice_free(struct release_cid_context, context);
}

int qmictl_release_cid(struct qmidev *qmidev, uint8_t service, uint8_t client,
                       qmidev_response_fn caller_callback,
                       void *caller_context)
{
  struct release_cid_context *context;
  int result;

  context = g_slice_new(struct release_cid_context);
  context->callback   = caller_callback;
  context->context    = caller_context;
  context->id.service = service;
  context->id.client  = client;

  result = qmidev_request(qmidev, &qmictl_service,
                          QMICTL_MSG_RELEASE_CLIENT_ID,
                          &release_cid_request, &context->id,
                          &release_cid_response, context);
  if (result)
    g_slice_free(struct release_cid_context, context);
  return result;
}
