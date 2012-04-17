/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmiwds: QMI calls on the WDS (Wireless Data Service) interface
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

struct qmiwds_priv {
  struct listener *packet_status_listener;
  qmidev_packet_status_callback_fn packet_status_callback;
  void *packet_status_context;
};

static int packet_status_changed(struct qmidev *qmidev,
                                 void *data,
                                 struct qmimsg *message)
{
  struct client *client = data;
  struct qmiwds_priv *priv = client_priv(client);
  uint16_t state = 0xfe;
  int result;

  /* unused */
  qmidev = qmidev;
  client = client;

  result = qmimsg_tlv_get(message, QMI_TLV_VALUE, sizeof(state), &state);

  if (priv->packet_status_callback)
    priv->packet_status_callback(qmidev,
                                 priv->packet_status_context,
                                 result,
                                 state);

  return 0;
}

static void client_create(struct qmidev *qmidev, struct client *client)
{
  struct listen_criteria criteria;

  qmidev = qmidev;
  struct qmiwds_priv *priv = g_slice_new(struct qmiwds_priv);

  criteria.flags = LISTEN_MATCH_QMI_FLAGS |
                   LISTEN_MATCH_MESSAGE;
  criteria.qmi_flags = QMI_FLAG_SVC_INDICATION;
  criteria.message = QMIWDS_MSG_PACKET_STATUS;

  priv->packet_status_listener = qmidev_listen(qmidev,
                                               client,
                                               &criteria,
                                               &packet_status_changed,
                                               client);

  priv->packet_status_callback = NULL;
  priv->packet_status_context = NULL;
  client_set_priv(client, priv);
}

static void client_destroy(struct qmidev *qmidev, struct client *client)
{
  qmidev = qmidev;
  struct qmiwds_priv *priv = client_priv(client);
  g_slice_free(struct qmiwds_priv, priv);
}

struct service qmiwds_service = {
  .service_id   = QMI_SVC_WDS,
  .event_msg_id = QMIWDS_MSG_EVENT,
  .create_fn    = &client_create,
  .destroy_fn   = &client_destroy
};

struct get_packet_status_context {
  qmidev_packet_status_response_fn callback;
  void *context;
};

static void get_packet_status_response(struct qmidev *qmidev,
                                       void *data,
                                       int result,
                                       struct qmimsg *message)
{
  struct get_packet_status_context *context = data;
  uint32_t state = 0;

  assert(qmidev);
  assert(data);

  if (result)
    goto out;

  result = qmimsg_tlv_get(message, QMI_TLV_VALUE, sizeof(state), &state);

out:
  context->callback(qmidev, context->context, result, state);
  g_slice_free(struct get_packet_status_context, context);
}

int qmidev_get_packet_status(struct qmidev *qmidev,
                             qmidev_packet_status_response_fn caller_callback,
                             void *caller_context)
{
  struct get_packet_status_context *context;
  int result;

  context = g_slice_new(struct get_packet_status_context);
  context->callback = caller_callback;
  context->context  = caller_context;

  result = qmidev_request(qmidev, &qmiwds_service,
                          QMIWDS_MSG_PACKET_STATUS,
                          NULL, NULL,
                          &get_packet_status_response, context);
  if (result)
    g_slice_free(struct get_packet_status_context, context);
  return result;
}

int qmidev_set_packet_status_callback(
    struct qmidev *qmidev,
    qmidev_packet_status_callback_fn callback,
    void *context,
    qmidev_response_fn set_callback,
    void *set_context)
{
  struct client *client = qmidev_default_client(qmidev, &qmiwds_service);
  struct qmiwds_priv *priv = client_priv(client);
  priv->packet_status_callback = callback;
  priv->packet_status_context = context;
  /* TODO: Call later. */
  if (set_callback)
    set_callback(qmidev, set_context, 0);
  return 0;
}

struct start_network_context {
  qmidev_start_network_response_fn callback;
  void *context;
};

enum {
  QMIWDS_TLV_START_NETWORK_CALL_END_REASON = 0x10,
  QMIWDS_TLV_START_NETWORK_VERBOSE_CALL_END_REASON = 0x11
};

static void start_network_response(struct qmidev *qmidev,
                                   void *data,
                                   int result,
                                   struct qmimsg *message)
{
  struct start_network_context *context = data;
  /* Arbitrary recognizable values, to debug whether the TLV is present. */
  qmidev_session_id session_id = 0xfefefefe;
  uint16_t call_end_type = 0xfefe;

  qmimsg_tlv_get(message, QMI_TLV_VALUE, sizeof(session_id), &session_id);
  qmimsg_tlv_get(message, QMIWDS_TLV_START_NETWORK_CALL_END_REASON,
                 sizeof(call_end_type), &call_end_type);

  /* TODO: Verbose call end reason? */

  context->callback(qmidev, context->context, result,
                    session_id, call_end_type);
  g_slice_free(struct start_network_context, context);
}

int qmidev_start_network(struct qmidev *qmidev,
                         qmidev_start_network_response_fn caller_callback,
                         void *caller_context)
{
  struct start_network_context *context;
  int result;

  context = g_slice_new(struct start_network_context);
  context->callback = caller_callback;
  context->context = caller_context;

  result = qmidev_request(qmidev, &qmiwds_service,
                          QMIWDS_MSG_START_NETWORK,
                          NULL, NULL,
                          &start_network_response, context);
  if (result)
    g_slice_free(struct start_network_context, context);
  return result;
}

static int stop_network_request(void *data,
                                struct qmimsg *message)
{
  qmidev_session_id *session_id = data;
  return qmimsg_tlv_add(message,
                        QMI_TLV_VALUE,
                        sizeof(*session_id),
                        session_id);
}

int qmidev_stop_network(struct qmidev *qmidev,
                        qmidev_session_id session_id,
                        qmidev_response_fn caller_callback,
                        void *caller_context)
{
  return qmidev_void_request(qmidev, &qmiwds_service,
                             QMIWDS_MSG_STOP_NETWORK,
                             &stop_network_request, &session_id,
                             caller_callback, caller_context);
}
