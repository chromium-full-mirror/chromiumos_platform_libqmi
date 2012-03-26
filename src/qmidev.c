/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "qmidev.h"

#include <assert.h>
#include <errno.h>
#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libqmi.h>

#include "dev.h"
#include "error.h"
#include "file.h"
#include "mock.h"
#include "poller.h"
#include "qmi.h"
#include "qmictl.h"
#include "qmimsg.h"
#include "util.h"

struct qmidev {
  void *priv;

  struct poller *poller;

  struct dev *dev;
  struct polled *dev_polled;
  int timerfd;
  struct polled *timer_polled;

  GList *callbacks;
  GList *listeners;
  GList *services;

  int connected;
  uint8_t control_tid;
};

struct qmisvc {
  uint8_t sid;
  uint8_t cid;
  uint16_t next_tid;
};

struct listener {
  struct listen_criteria criteria;
  qmidev_listen_callback_fn callback;
  void *context;
};

struct callback {
  callback_fn func;
  void *context;
};

static void call_listeners(struct qmidev *qmidev, struct qmimsg *message);
static void call_callbacks(struct qmidev *qmidev);

static struct qmidev *qmidev_new(void)
{
  struct qmidev *qmidev = g_slice_new(struct qmidev);

  qmidev->priv = NULL;

  qmidev->poller = poller_new();
  qmidev->dev = NULL;
  qmidev->dev_polled = NULL;

  qmidev->callbacks = NULL;
  qmidev->listeners = NULL;

  qmidev->connected = 0;
  qmidev->control_tid = 1;

  return qmidev;
}

struct qmidev *qmidev_new_file(const char *path)
{
  assert(path);

  struct qmidev *qmidev = qmidev_new();

  struct dev *dev = file_new(qmidev, path);
  if (!dev) {
    free(qmidev);
    return NULL;
  }
  qmidev->dev = dev;

  return qmidev;
}

struct qmidev *qmidev_new_mock(void)
{
  struct qmidev *qmidev = qmidev_new();

  struct dev *dev = mock_new();
  qmidev->dev = dev;

  return qmidev;
}

void *qmidev_priv(struct qmidev *qmidev)
{
  assert(qmidev);
  return qmidev->priv;
}

void qmidev_set_priv(struct qmidev *qmidev, void *priv)
{
  assert(qmidev);
  qmidev->priv = priv;
}

int qmidev_fd(struct qmidev *qmidev)
{
  assert(qmidev);
  return poller_fd(qmidev->poller);
}

int qmidev_process(struct qmidev *qmidev)
{
  assert(qmidev);

  call_callbacks(qmidev);
  return poller_poll(qmidev->poller);
}

static struct qmisvc *qmisvc_alloc(uint8_t sid)
{
  struct qmisvc *svc = g_slice_new(struct qmisvc);
  svc->sid = sid;
  svc->cid = QMI_CID_NONE;
  svc->next_tid = 1;
  return svc;
}

static void qmisvc_free(struct qmisvc *svc)
{
  g_slice_free(struct qmisvc, svc);
}

struct connect_context {
  GQueue *pending_services;
  struct qmisvc *current_service;

  qmidev_response_fn caller_callback;
  void *caller_context;
};

static void connect_next_service(struct qmidev *qmidev,
                                 struct connect_context *context);

static void got_cid(struct qmidev *qmidev,
                    void *data,
                    int status,
                    uint8_t cid)
{
  struct connect_context *context = data;

  if (!status) {
    context->current_service->cid = cid;
    qmidev->services = g_list_prepend(qmidev->services,
                                      context->current_service);
  } else {
    qmisvc_free(context->current_service);
  }

  connect_next_service(qmidev, context);
}

static void connect_next_service(struct qmidev *qmidev,
                                 struct connect_context *context)
{
  if (g_queue_is_empty(context->pending_services)) {
    context->caller_callback(qmidev, context->caller_context, 0);
    g_queue_free(context->pending_services);
    g_slice_free(struct connect_context, context);
    return;
  }

  context->current_service = g_queue_pop_head(context->pending_services);
  /* TODO: Check return */
  qmictl_get_cid(qmidev, context->current_service->sid,
                 &got_cid, context);
}

int qmidev_connect(struct qmidev *qmidev,
                   qmidev_response_fn caller_callback,
                   void *caller_context)
{
  assert(qmidev);
  assert(caller_callback);

  if (qmidev->connected)
    return QMI_ERR_ALREADY_CONNECTED;

  int result = dev_open(qmidev->dev);
  /* TODO: Should this go through the callback instead? */
  if (result)
    return result;

  qmidev->connected = 1;

  struct connect_context *context = g_slice_new(struct connect_context);
  context->caller_callback = caller_callback;
  context->caller_context = caller_context;
  context->current_service = NULL;
  context->pending_services = g_queue_new();

  qmidev->services = g_list_append(qmidev->services, qmisvc_alloc(QMI_SVC_CTL));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_WDS));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_DMS));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_NAS));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_QOS));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_WMS));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_PDS));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_AUTH));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_VOICE));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_CAT));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_RMS));
  g_queue_push_tail(context->pending_services, qmisvc_alloc(QMI_SVC_OMA));

  connect_next_service(qmidev, context);

  return 0;
}

struct disconnect_context {
  struct qmisvc *current_service;

  qmidev_response_fn caller_callback;
  void *caller_context;
};

static void services_disconnected(struct qmidev *qmidev,
                                  struct disconnect_context *context)
{
  int result = dev_close(qmidev->dev);
  qmidev->connected = 0;
  context->caller_callback(qmidev, context->caller_context, result);
  g_slice_free(struct disconnect_context, context);
}

static void disconnect_next_service(struct qmidev *qmidev,
                                    struct disconnect_context *context);

static void released_cid(struct qmidev *qmidev,
                         void *data,
                         int status)
{
  struct disconnect_context *context = data;

  if (status) {
    /* TODO: What do we do if release fails? */
  }

  qmisvc_free(context->current_service);
  disconnect_next_service(qmidev, context);
}

static void disconnect_next_service(struct qmidev *qmidev,
                                    struct disconnect_context *context)
{
  if (!qmidev->services) {
    services_disconnected(qmidev, context);
    return;
  }

  context->current_service = qmidev->services->data;
  qmidev->services = g_list_remove(qmidev->services, context->current_service);
  /* TODO: Check return */
  if (context->current_service->sid != QMI_SVC_CTL) {
    qmictl_release_cid(qmidev, context->current_service->sid,
                       context->current_service->cid,
                       &released_cid, context);
  } else {
    qmisvc_free(context->current_service);
    disconnect_next_service(qmidev, context);
  }
}

int qmidev_disconnect(struct qmidev *qmidev,
                      qmidev_response_fn caller_callback,
                      void *caller_context)
{
  assert(qmidev);
  assert(caller_callback);

  if (!qmidev->connected)
    return QMI_ERR_NOT_CONNECTED;

  struct disconnect_context *context = g_slice_new(struct disconnect_context);
  context->caller_callback = caller_callback;
  context->caller_context = caller_context;
  context->current_service = NULL;

  disconnect_next_service(qmidev, context);

  return 0;
}

gint service_compare_sid(gconstpointer a, gconstpointer b)
{
  const struct qmisvc *svc_a = a;
  const struct qmisvc *svc_b = b;

  return svc_a->sid - svc_b->sid;
}

struct qmisvc *find_service(struct qmidev *qmidev, uint8_t service_id)
{
  struct qmisvc wanted;
  wanted.sid = service_id;

  GList *found_list = g_list_find_custom(qmidev->services, &wanted,
                                         &service_compare_sid);
  if (found_list)
    return (struct qmisvc *) found_list->data;
  else
    return NULL;
}

uint16_t get_free_tid(struct qmisvc *service)
{
  uint16_t next_tid = service->next_tid;

  /* CTL uses 8-bit transaction IDs; other services use 16-bit. */
  uint16_t max_tid = (service->sid == QMI_SVC_CTL) ? 0x00FF : 0xFFFF;
  if (service->next_tid < max_tid)
    service->next_tid++;
  else
    service->next_tid = 0x0001;

  return next_tid;
}

int qmidev_make_request(struct qmidev *qmidev,
                        uint8_t service_id,
                        uint16_t req_msg_id,
                        struct qmimsg **message_out)
{
  struct qmisvc *service;
  uint16_t transaction_id;
  struct qmimsg *message;
  int result;

  assert(qmidev);
  assert(message_out);

  service = find_service(qmidev, service_id);
  if (!service)
    return QMI_ERR_SERVICE_UNAVAILABLE;

  transaction_id = get_free_tid(service);

  result = qmimsg_new(0, service_id, service->cid,
                      0, transaction_id, req_msg_id,
                      &message);
  if (result)
    return result;

  *message_out = message;

  return 0;
}

struct request_context {
  uint16_t resp_msg_id;

  qmidev_request_fn caller_callback;
  void *caller_context;
};

int got_reply(struct qmidev *qmidev, void *data, struct qmimsg *message)
{
  struct request_context *context = data;
  int result;

  /* TODO: Check response message ID. */

  result = qmi_result(message);
  if (result)
    goto out;

out:
  context->caller_callback(qmidev, context->caller_context, result, message);
  g_slice_free(struct request_context, context);
  return 1;
}

int qmidev_send_request(struct qmidev *qmidev,
                        struct qmimsg *req_msg,
                        uint16_t resp_msg_id,
                        qmidev_request_fn caller_callback,
                        void *caller_context)
{
  assert(qmidev);
  assert(req_msg);
  assert(caller_callback);

  if (!qmidev->connected)
    return QMI_ERR_NOT_CONNECTED;

  struct request_context *context = g_slice_new(struct request_context);

  context->caller_callback = caller_callback;
  context->caller_context = caller_context;
  context->resp_msg_id = resp_msg_id;

  struct listen_criteria criteria;
  qmimsg_get_header(req_msg,
                    &criteria.service,
                    &criteria.client,
                    &criteria.transaction);
  struct listener *listener = qmidev_listen(qmidev, &criteria,
                                            &got_reply, context);

  /* TODO: Do something with listener?  Currently we don't need it for
     anything, but once we add timeouts we'll need to be able to cancel
     it. */

  int result = qmidev_send(qmidev, req_msg);
  if (result) {
    qmidev_cancel_listen(qmidev, listener);
    g_slice_free(struct request_context, context);
    qmimsg_free(req_msg);
    return result;
  }

  return 0;
}

static int dev_read_fn(void *ctx, void *buf, size_t len)
{
  assert(ctx);
  struct qmidev *qmidev = ctx;
  assert(qmidev->dev);

  return dev_read(qmidev->dev, buf, len);
}

static void dev_read_cb(struct polled *polled)
{
  assert(polled);
  struct qmidev *qmidev = polled_priv(polled);
  assert(qmidev);

  struct qmimsg *msg;
  int result = qmimsg_read(&dev_read_fn, qmidev, &msg);
  /* TODO: Do something. */
  if (result)
    return;
  call_listeners(qmidev, msg);
  qmimsg_free(msg);
}

static void dev_close_cb(struct polled *polled)
{
  assert(polled);
  struct qmidev *qmidev = polled_priv(polled);
  assert(qmidev);

  qmidev_clear_dev_fd(qmidev);
}

int qmidev_set_dev_fd(struct qmidev *qmidev, int fd)
{
  assert(qmidev);
  assert(fd >= 0);
  assert(!qmidev->dev_polled);

  qmidev->dev_polled = poller_add(qmidev->poller, fd);
  if (!qmidev->dev_polled)
    return QMI_ERR_INTERNAL;
  polled_set_priv(qmidev->dev_polled, qmidev);
  polled_set_read(qmidev->dev_polled, &dev_read_cb);
  polled_set_close(qmidev->dev_polled, &dev_close_cb);
  polled_update(qmidev->dev_polled);

  return 0;
}

int qmidev_clear_dev_fd(struct qmidev *qmidev)
{
  assert(qmidev);
  assert(qmidev->dev_polled);

  /* Make sure we don't get called again. */
  polled_set_close(qmidev->dev_polled, NULL);
  polled_update(qmidev->dev_polled);

  /* will be freed by the poller when the fd closes. */
  qmidev->dev_polled = NULL;

  return 0;
}

static int dev_write_fn(void *ctx, const void *buf, size_t len)
{
  assert(ctx);
  struct qmidev *qmidev = ctx;
  assert(qmidev->dev);

  return dev_write(qmidev->dev, buf, len);
}

int qmidev_send(struct qmidev *qmidev, struct qmimsg *msg)
{
  assert(qmidev);
  assert(qmidev->connected);

  return qmimsg_write(msg, &dev_write_fn, qmidev);
}

struct polled *qmidev_poll(struct qmidev *qmidev, int fd)
{
  assert(qmidev);

  return poller_add(qmidev->poller, fd);
}



void qmidev_call_later(struct qmidev *qmidev, callback_fn func, void *context)
{
  assert(qmidev);
  assert(func);

  struct callback *callback = g_slice_new(struct callback);
  callback->func = func;
  callback->context = context;
  qmidev->callbacks = g_list_append(qmidev->callbacks, callback);
}

static void call_callback(void *data, void *user_data)
{
  struct qmidev *qmidev = user_data;
  struct callback *callback = data;

  callback->func(qmidev, callback->context);
  g_slice_free(struct callback, callback);
}

static void call_callbacks(struct qmidev *qmidev)
{
  g_list_foreach(qmidev->callbacks, call_callback, qmidev);
  g_list_free(qmidev->callbacks);
  qmidev->callbacks = NULL;
}



struct listener *qmidev_listen(struct qmidev *qmidev,
                               struct listen_criteria *criteria,
                               qmidev_listen_callback_fn callback,
                               void *context)
{
  assert(qmidev);
  assert(criteria);
  assert(callback);

  struct listener *listener = g_slice_new(struct listener);

  listener->criteria = *criteria;
  listener->callback = callback;
  listener->context = context;

  qmidev->listeners = g_list_prepend(qmidev->listeners, listener);

  return listener;
}

void qmidev_cancel_listen(struct qmidev *qmidev, struct listener *listener)
{
  assert(qmidev);
  assert(listener);

  qmidev->listeners = g_list_remove(qmidev->listeners, listener);
}

struct call_listener_context {
  struct qmidev *qmidev;
  struct qmimsg *message;
  uint8_t service;
  uint8_t client;
  uint16_t transaction;
};

static void call_listener(void *data, void *user_data)
{
  struct call_listener_context *context = user_data;
  struct listener *listener = data;

  if (listener->criteria.service != context->service)
    return;

  if ((listener->criteria.client != QMI_CID_BROADCAST) &&
      (listener->criteria.client != context->client))
    return;

  if ((listener->criteria.transaction != 0) &&
      (listener->criteria.transaction != context->transaction))
    return;

  int result = listener->callback(context->qmidev,
                                  listener->context,
                                  context->message);
  if (result) {
    /* The glib documentation does not note this, but it is in safe to call
       g_list_remove while called back from g_list_foreach. */
    qmidev_cancel_listen(context->qmidev, listener);
  }
}

static void call_listeners(struct qmidev *qmidev, struct qmimsg *message)
{
  struct call_listener_context context;
  context.qmidev = qmidev;
  context.message = message;
  qmimsg_get_header(message,
                    &context.service, &context.client, &context.transaction);
  g_list_foreach(qmidev->listeners, &call_listener, &context);
}
