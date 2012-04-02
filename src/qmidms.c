/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmidms: QMI calls on the DMS (Device Management Service) interface
 *
 * Sources used in writing this file (see README for links):
 * [GobiNet]/QMI.c
 * [GobiNet]/QMIDevice.c
 * [cros-kerne]/drivers/net/usb/gobi/qmi.c
 * [cros-kerne]/drivers/net/usb/gobi/qmidevice.c
 */

#include <assert.h>
#include <errno.h>
#include <glib.h>

#include <libqmi.h>

#include "error.h"
#include "qmi.h"
#include "qmidev.h"
#include "qmimsg.h"

struct qmidms_priv {
  struct event operating_mode;
};

static void operating_mode_changed(struct qmidev *qmidev,
                                   void *user_callback, void *user_context,
                                   uint16_t length, void *value)
{
  qmidev_power_callback_fn callback = user_callback;
  qmidev_power_state state;
  int status;

  if (length == 1) {
    state = *(uint8_t *)value;
    status = 0;
  } else {
    state = 0;
    status = QMI_ERR_MESSAGE_INVALID;
  }

  callback(qmidev, user_context, status, state);
}

static void client_create(struct qmidev *qmidev, struct client *client)
{
  qmidev = qmidev;
  struct qmidms_priv *priv = g_slice_new(struct qmidms_priv);
  event_init(&priv->operating_mode,
             QMIDMS_EVENT_OPERATING_MODE,
             &operating_mode_changed);
  client_set_priv(client, priv);
}

static void client_destroy(struct qmidev *qmidev, struct client *client)
{
  qmidev = qmidev;
  struct qmidms_priv *priv = client_priv(client);
  g_slice_free(struct qmidms_priv, priv);
}

struct service qmidms_service = {
  .service_id   = QMI_SVC_DMS,
  .event_msg_id = QMIDMS_MSG_EVENT,
  .create_fn    = &client_create,
  .destroy_fn   = &client_destroy
};enum {
  QMIDMS_TLV_GET_IDS_ESN = 0x10,
  QMIDMS_TLV_GET_IDS_IMEI,
  QMIDMS_TLV_GET_IDS_MEID
};

struct get_ids_context {
  qmidev_get_ids_response_fn callback;
  void *context;
};

static void get_ids_response(struct qmidev *qmidev,
                             void *data,
                             int result,
                             struct qmimsg *message)
{
  struct get_ids_context *context = data;
  char *esn = NULL, *imei = NULL, *meid = NULL;

  assert(qmidev);
  assert(data);

  if (result)
    goto out;

  qmi_tlv_strdup(message, QMIDMS_TLV_GET_IDS_ESN,  NULL, &esn);
  qmi_tlv_strdup(message, QMIDMS_TLV_GET_IDS_IMEI, NULL, &imei);
  qmi_tlv_strdup(message, QMIDMS_TLV_GET_IDS_MEID, NULL, &meid);

out:
  context->callback(qmidev, context->context, result,
                    esn, imei, meid);
  g_free(esn);
  g_free(imei);
  g_free(meid);
  g_slice_free(struct get_ids_context, context);
}

int qmidev_get_ids(struct qmidev *qmidev,
                   qmidev_get_ids_response_fn caller_callback,
                   void *caller_context)
{
  struct get_ids_context *context;
  int result;

  context = g_slice_new(struct get_ids_context);
  context->callback = caller_callback;
  context->context  = caller_context;

  result = qmidev_request(qmidev, &qmidms_service,
                          QMIDMS_MSG_GET_IDS,
                          NULL, NULL,
                          &get_ids_response, context);
  if (result)
    g_slice_free(struct get_ids_context, context);
  return result;
}

struct get_power_context {
  qmidev_power_state_response_fn caller_callback;
  void *caller_context;
};

static void get_power_response(struct qmidev *qmidev,
                               void *data,
                               int result,
                               struct qmimsg *message)
{
  struct get_power_context *context = data;
  uint8_t mode;

  assert(qmidev);
  assert(data);

  if (result)
    goto out;

  result = qmimsg_tlv_get(message, QMI_TLV_VALUE, sizeof(mode), &mode);
  if (result)
    goto out;

out:
  context->caller_callback(qmidev, context->caller_context, result, mode);
  g_slice_free(struct get_power_context, context);
}

int qmidev_get_power(struct qmidev *qmidev,
                     qmidev_power_state_response_fn caller_callback,
                     void *caller_context)
{
  struct get_power_context *context;
  int result;

  context = g_slice_new(struct get_power_context);
  context->caller_callback = caller_callback;
  context->caller_context  = caller_context;

  result = qmidev_request(qmidev, &qmidms_service,
                          QMIDMS_MSG_GET_OPERATING_MODE,
                          NULL, NULL,
                          &get_power_response, context);
  if (result)
    g_slice_free(struct get_power_context, context);
  return result;
}

static int set_power_request(void *data,
                             struct qmimsg *message)
{
  uint8_t *mode = data;
  return qmimsg_tlv_add(message, QMI_TLV_VALUE, sizeof(*mode), mode);
}

int qmidev_set_power(struct qmidev *qmidev,
                     qmidev_power_state state,
                     qmidev_response_fn callback,
                     void *context)
{
  uint8_t mode = state;

  return qmidev_void_request(qmidev, &qmidms_service,
                             QMIDMS_MSG_SET_OPERATING_MODE,
                             &set_power_request, &mode,
                             callback, context);
}



int qmidev_set_power_callback(struct qmidev *qmidev,
                              qmidev_power_callback_fn callback,
                              void *context,
                              qmidev_response_fn set_callback,
                              void *set_context)
{
  struct client *client = qmidev_default_client(qmidev, &qmidms_service);
  struct qmidms_priv *priv = client_priv(client);
  return qmidev_set_callback(qmidev, client, &priv->operating_mode,
                             callback, context,
                             set_callback, set_context);
}
