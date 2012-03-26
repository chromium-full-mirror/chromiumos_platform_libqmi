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

#include "qmi.h"
#include "qmidev.h"
#include "qmimsg.h"

enum {
  QMIDMS_MSG_GET_IDS = 0x0025,
  QMIDMS_MSG_GET_OPERATING_MODE = 0x002D,
  QMIDMS_MSG_SET_OPERATING_MODE = 0x002E
};

enum {
  QMIDMS_TLV_GET_IDS_ESN = 0x10,
  QMIDMS_TLV_GET_IDS_IMEI,
  QMIDMS_TLV_GET_IDS_MEID
};

struct get_ids_context {
  qmidev_get_ids_response_fn caller_callback;
  void *caller_context;
};

static void got_ids(struct qmidev *qmidev,
                    void *data,
                    int status,
                    struct qmimsg *message)
{
  assert(qmidev);
  assert(data);

  struct get_ids_context *context = data;
  int result;

  char *esn = NULL, *imei = NULL, *meid = NULL;

  if (status) {
    result = status;
    goto out;
  }

  qmi_tlv_strdup(message, QMIDMS_TLV_GET_IDS_ESN,  NULL, &esn);
  qmi_tlv_strdup(message, QMIDMS_TLV_GET_IDS_IMEI, NULL, &imei);
  qmi_tlv_strdup(message, QMIDMS_TLV_GET_IDS_MEID, NULL, &meid);

  result = 0;

out:
  context->caller_callback(qmidev, context->caller_context, result,
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
  int result;

  struct get_ids_context *context = g_slice_new(struct get_ids_context);
  context->caller_callback = caller_callback;
  context->caller_context  = caller_context;

  struct qmimsg *message;
  result = qmidev_make_request(qmidev, QMI_SVC_DMS, QMIDMS_MSG_GET_IDS,
                               &message);
  if (result)
    goto fail;

  result = qmidev_send_request(qmidev, message,
                               QMIDMS_MSG_GET_IDS,
                               &got_ids, context);
  if (result)
    goto fail;

  return 0;

fail:
  g_slice_free(struct get_ids_context, context);
  return result;
}

struct get_power_context {
  qmidev_power_state_response_fn caller_callback;
  void *caller_context;
};

static void got_power(struct qmidev *qmidev,
                      void *data,
                      int status,
                      struct qmimsg *message)
{
  uint8_t mode;

  assert(qmidev);
  assert(data);

  struct get_power_context *context = data;
  int result;

  if (status) {
    result = status;
    goto out;
  }

  result = qmimsg_tlv_get(message, QMI_TLV_VALUE, sizeof(mode), &mode);
  if (result)
    goto out;

  result = 0;

out:
  context->caller_callback(qmidev, context->caller_context, result, mode);
  g_slice_free(struct get_power_context, context);
}

int qmidev_get_power(struct qmidev *qmidev,
                     qmidev_power_state_response_fn caller_callback,
                     void *caller_context)
{
  int result;

  struct get_power_context *context = g_slice_new(struct get_power_context);
  context->caller_callback = caller_callback;
  context->caller_context  = caller_context;

  struct qmimsg *message;
  result = qmidev_make_request(qmidev,
                               QMI_SVC_DMS,
                               QMIDMS_MSG_GET_OPERATING_MODE,
                               &message);
  if (result)
    goto fail;

  result = qmidev_send_request(qmidev, message,
                               QMIDMS_MSG_GET_OPERATING_MODE,
                               &got_power, context);
  if (result)
    goto fail;

  return 0;

fail:
  g_slice_free(struct get_power_context, context);
  return result;
}

struct set_power_context {
  qmidev_response_fn caller_callback;
  void *caller_context;
};

static void set_power(struct qmidev *qmidev,
                         void *data,
                         int status,
                         struct qmimsg *message)
{
  assert(qmidev);
  assert(data);

  struct set_power_context *context = data;
  int result;

  if (status) {
    result = status;
    goto out;
  }

  message = message;
  result = 0;

out:
  context->caller_callback(qmidev, context->caller_context, result);
  g_slice_free(struct set_power_context, context);
}

int qmidev_set_power(struct qmidev *qmidev,
                     qmidev_power_state state,
                     qmidev_response_fn caller_callback,
                     void *caller_context)
{
  uint8_t mode = state;
  int result;

  struct set_power_context *context = g_slice_new(struct set_power_context);
  context->caller_callback = caller_callback;
  context->caller_context  = caller_context;

  struct qmimsg *message;
  result = qmidev_make_request(qmidev,
                               QMI_SVC_DMS,
                               QMIDMS_MSG_SET_OPERATING_MODE,
                               &message);
  qmimsg_tlv_add(message, QMI_TLV_VALUE, sizeof(mode), &mode);
  result = qmidev_send_request(qmidev, message,
                               QMIDMS_MSG_SET_OPERATING_MODE,
                               &set_power, context);
  if (result)
    goto fail;

  return 0;

fail:
  g_slice_free(struct set_power_context, context);
  return result;
}
