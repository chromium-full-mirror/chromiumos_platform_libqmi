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
#include <stdio.h>
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
#include "qmidev.h"
#include "qmidms.h"
#include "qmimsg.h"
#include "util.h"

static const int qmidev_log_unhandled_messages = 0;

struct qmidev {
  void *priv;

  struct poller *poller;

  struct dev *dev;
  struct polled *dev_polled;

  GList *callbacks;
  GList *listeners;
  GList *clients;

  int connected;
};

enum {
  MATCH_SERVICE     = 0x01,
  MATCH_CLIENT      = 0x02,
  MATCH_QMI_FLAGS   = 0x04,
  MATCH_TRANSACTION = 0x08,
  MATCH_MESSAGE     = 0x10
};

/**
 * Callback type for qmidev_listen.
 *
 * Will be called when a message matching the listen criteria is received.
 *
 * @qmidev: the QMI device on which a message was received
 * @context: the context passed to qmidev_listen
 * @message: the message received
 *
 * The callback should return zero to continue listening for such messages,
 * or non-zero to remove the listener.
 */
typedef int (*qmidev_listen_callback_fn)(struct qmidev *qmidev,
                                         void *context,
                                         struct qmimsg *message);

struct listen_criteria {
  int flags;
  uint8_t service;
  uint8_t client;
  uint8_t qmi_flags;
  uint16_t transaction;
  uint16_t message;
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

static int service_is_control(struct service *service)
{
  return !service->service_id;
}

static int call_listeners(struct qmidev *qmidev, struct qmimsg *message);
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
  qmidev->clients = NULL;

  qmidev->connected = 0;

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
  g_slice_free(struct listener, listener);
}

struct call_listener_context {
  struct qmidev *qmidev;
  struct qmimsg *message;
  uint8_t msg_service;
  uint8_t msg_client;
  uint8_t msg_qmi_flags;
  uint16_t msg_transaction;
  uint16_t msg_message;
  int calls;
};

static void call_listener(void *data, void *user_data)
{
  struct call_listener_context *context = user_data;
  struct listener *listener = data;
  struct listen_criteria *crit = &listener->criteria;
  int flags = crit->flags;

  if ((flags & MATCH_SERVICE) &&
      (crit->service != context->msg_service))
    return;

  if ((flags & MATCH_CLIENT) &&
      (crit->client != context->msg_client))
    return;

  if ((flags & MATCH_QMI_FLAGS) &&
      (crit->qmi_flags != context->msg_qmi_flags))
    return;

  if ((flags & MATCH_TRANSACTION) &&
      (crit->transaction != context->msg_transaction))
    return;

  if ((flags & MATCH_MESSAGE) &&
      (crit->message != context->msg_message))
    return;

  int result = listener->callback(context->qmidev,
                                  listener->context,
                                  context->message);
  if (result) {
    /* The glib documentation does not note this, but it is in fact safe to
       call g_list_remove while called back from g_list_foreach. */
    qmidev_cancel_listen(context->qmidev, listener);
  }

  context->calls++;
}

static int call_listeners(struct qmidev *qmidev, struct qmimsg *message)
{
  struct call_listener_context context;
  context.qmidev = qmidev;
  context.message = message;
  qmimsg_get_header(message,
                    &context.msg_service,
                    &context.msg_client,
                    &context.msg_qmi_flags,
                    &context.msg_transaction,
                    &context.msg_message);
  context.calls = 0;
  g_list_foreach(qmidev->listeners, &call_listener, &context);
  return context.calls == 0;
}



struct client {
  struct service *service;

  uint8_t client_id;
  uint16_t next_transaction_id;
  struct listener *event_listener;
  GList *events;

  void *priv;
};

static struct client *make_ctl_client(struct qmidev *qmidev)
{
  struct client *client = g_slice_new(struct client);

  client->service = &qmictl_service;
  client->client_id = 0x00;
  client->next_transaction_id = 0x0001;
  client->events = NULL;
  client->priv = NULL;

  if (client->service->create_fn)
    client->service->create_fn(qmidev, client);

  return client;
}

struct transaction {
  struct client *client;
  uint16_t transaction_id;

  struct listener *response_listener;
  response_fn callback;
  void *context;
};

struct call_event_listeners_context {
  struct qmidev *qmidev;
  struct client *client;
  uint8_t type;
  uint16_t length;
  void *value;
  int calls;
};

static void call_event_listener(void *data, void *user_data)
{
  struct call_event_listeners_context *context = user_data;
  struct event *event = data;

  if (event->type != context->type)
    return;

  event->callback(context->qmidev,
                  event->user_callback,
                  event->user_context,
                  context->length,
                  context->value);

  context->calls++;
}

static void call_event_listeners(void *data,
                                 uint8_t type, uint16_t length, void *value)
{
  struct call_event_listeners_context *context = data;

  context->type = type;
  context->length = length;
  context->value = value;
  context->calls = 0;

  g_list_foreach(context->client->events, &call_event_listener, context);
}

static int event_message_received(struct qmidev *qmidev,
                                   void *data,
                                   struct qmimsg *message)
{
  struct call_event_listeners_context context;

  context.qmidev = qmidev;
  context.client = data;
  qmimsg_tlv_foreach(message, &call_event_listeners, &context);

  return 0;
}

struct get_client_context {
  struct client *client;
  qmidev_client_fn callback;
  void *context;
};

static void got_cid(struct qmidev *qmidev,
                    void *data,
                    int status,
                    uint8_t cid)
{
  struct get_client_context *context = data;
  struct client *client = context->client;
  struct listen_criteria event_criteria;

  if (status) {
    g_slice_free(struct client, client);
    client = NULL;
    goto out;
  }

  client->client_id = cid;
  if (client->service->create_fn)
    client->service->create_fn(qmidev, client);

  event_criteria.flags = MATCH_SERVICE | MATCH_CLIENT | MATCH_QMI_FLAGS |
                         MATCH_MESSAGE;
  event_criteria.service = client->service->service_id;
  event_criteria.client = client->client_id;
  if (service_is_control(client->service))
    event_criteria.qmi_flags = QMI_FLAG_CTL_INDICATION;
  else
    event_criteria.qmi_flags = QMI_FLAG_SVC_INDICATION;
  event_criteria.message = client->service->event_msg_id;
  client->event_listener = qmidev_listen(qmidev,
                                         &event_criteria,
                                         &event_message_received,
                                         client);

out:
  context->callback(qmidev, context->context, status, client);
  g_slice_free(struct get_client_context, context);
}

int qmidev_get_client(struct qmidev *qmidev,
                      struct service *service,
                      qmidev_client_fn caller_callback,
                      void *caller_context)
{
  struct client *client;
  struct get_client_context *context;
  int result;

  client = g_slice_new(struct client);
  client->service = service;
  client->next_transaction_id = 0x0001;
  client->events = NULL;
  client->priv = NULL;

  context = g_slice_new(struct get_client_context);
  context->client = client;
  context->callback = caller_callback;
  context->context = caller_context;

  result = qmictl_get_cid(qmidev,
                          client->service->service_id,
                          &got_cid,
                          context);
  if (result) {
    g_slice_free(struct get_client_context, context);
    g_slice_free(struct client, client);
  }

  return result;
}

struct release_client_context {
  struct client *client;
  qmidev_response_fn callback;
  void *context;
};

static void released_cid(struct qmidev *qmidev,
                         void *data,
                         int status)
{
  struct release_client_context *context = data;
  struct client *client = context->client;

  if (status)
    goto out;

  qmidev_cancel_listen(qmidev, client->event_listener);

  /* TODO: Free things that reference client (transactions). */

  if (context->client->service->destroy_fn)
      context->client->service->destroy_fn(qmidev, context->client);
  g_slice_free(struct client, context->client);

out:
  context->callback(qmidev, context->context, status);
  g_slice_free(struct release_client_context, context);
}

int qmidev_release_client(struct qmidev *qmidev,
                          struct client *client,
                          qmidev_response_fn caller_callback,
                          void *caller_context)
{
  struct release_client_context *context;
  int result;

  context = g_slice_new(struct release_client_context);
  context->client = client;
  context->callback = caller_callback;
  context->context = caller_context;

  result = qmictl_release_cid(qmidev,
                              client->service->service_id,
                              client->client_id,
                              &released_cid,
                              context);
  if (result)
    g_slice_free(struct release_client_context, context);

  return result;
}

static gint client_compare_service(gconstpointer a, gconstpointer b)
{
  const struct client *client_a = a;
  const struct client *client_b = b;

  return client_a->service - client_b->service;
}

static struct client *find_client(struct qmidev *qmidev,
                                  struct service *service)
{
  struct client wanted;
  GList *found_list;

  wanted.service = service;
  found_list = g_list_find_custom(qmidev->clients,
                                  &wanted,
                                  &client_compare_service);

  if (found_list)
    return (struct client *) found_list->data;
  else
    return NULL;
}

struct client *qmidev_default_client(struct qmidev *qmidev,
                                     struct service *service)
{
  return find_client(qmidev, service);
}

static struct transaction *make_transaction(struct client *client,
                                            response_fn callback,
                                            void *context)
{
  struct transaction *transaction;

  transaction = g_slice_new(struct transaction);
  transaction->client = client;
  transaction->transaction_id = client->next_transaction_id;
  transaction->callback = callback;
  transaction->context = context;

  return transaction;
}

static void increment_transaction_id(struct client *client)
{
  static const uint16_t min_transaction_id = 0x0001;
  uint16_t max_transaction_id;

  if (client->service->service_id == QMI_SVC_CTL)
    max_transaction_id = 0x00FF;
  else
    max_transaction_id = 0xFFFF;

  if (client->next_transaction_id < max_transaction_id)
    client->next_transaction_id++;
  else
    client->next_transaction_id = min_transaction_id;
}

static int response_received(struct qmidev *qmidev,
                             void *data,
                             struct qmimsg *message)
{
  struct transaction *transaction = data;

  transaction->callback(qmidev,
                        transaction->context,
                        qmi_result(message),
                        message);
  g_slice_free(struct transaction, transaction);

  /* remove listener */
  return 1;
}

int qmidev_client_request(struct qmidev *qmidev,
                          struct client *client,
                          uint16_t msg_id,
                          request_fn make_request_fn,
                          void *request_context,
                          response_fn handle_response_fn,
                          void *response_context)
{
  struct transaction *transaction;
  struct qmimsg *request_message;
  struct listen_criteria response_criteria;
  int result;

  transaction = make_transaction(client, handle_response_fn, response_context);

  qmimsg_new(0, client->service->service_id, client->client_id,
             0, transaction->transaction_id, msg_id,
             &request_message);
  if (make_request_fn) {
    result = make_request_fn(request_context, request_message);
    if (result)
      goto fail;
  }

  response_criteria.flags = MATCH_SERVICE | MATCH_CLIENT | MATCH_TRANSACTION |
                            MATCH_QMI_FLAGS | MATCH_MESSAGE;
  response_criteria.service = client->service->service_id;
  response_criteria.client = client->client_id;
  if (service_is_control(client->service))
    response_criteria.qmi_flags = QMI_FLAG_CTL_RESPONSE;
  else
    response_criteria.qmi_flags = QMI_FLAG_SVC_RESPONSE;
  response_criteria.transaction = client->next_transaction_id;
  response_criteria.message = msg_id;

  transaction->response_listener =
          qmidev_listen(qmidev,
                        &response_criteria,
                        &response_received,
                        transaction);

  /* TODO: Timeout. */

  result = qmidev_send(qmidev, request_message);

  increment_transaction_id(client);

  return 0;

fail:
  g_slice_free(struct transaction, transaction);
  qmimsg_free(request_message);
  return result;
}

int qmidev_request(struct qmidev *qmidev,
                   struct service *service,
                   uint16_t msg_id,
                   request_fn make_request_fn,
                   void *request_context,
                   response_fn handle_response_fn,
                   void *response_context)
{
  struct client *client;

  client = find_client(qmidev, service);
  if (!client)
    return QMI_ERR_SERVICE_UNAVAILABLE;

  return qmidev_client_request(qmidev, client, msg_id,
                               make_request_fn, request_context,
                               handle_response_fn, response_context);
}

struct void_context {
  qmidev_response_fn callback;
  void *context;
};

static void void_response_received(struct qmidev *qmidev,
                                   void *data,
                                   int result,
                                   struct qmimsg *message)
{
  struct void_context *context = data;

  /* nothing useful in message. */
  message = message;

  context->callback(qmidev, context->context, result);
  g_slice_free(struct void_context, context);
}

int qmidev_void_request(struct qmidev *qmidev,
                        struct service *service,
                        uint16_t msg_id,
                        request_fn make_request_fn,
                        void *request_context,
                        qmidev_response_fn handle_response_fn,
                        void *response_context)
{
  struct void_context *context;
  int result;

  context = g_slice_new(struct void_context);
  context->callback = handle_response_fn;
  context->context = response_context;

  result = qmidev_request(qmidev, service, msg_id,
                          make_request_fn, request_context,
                          &void_response_received, context);
  if (result)
    g_slice_free(struct void_context, context);
  return result;
}

void *client_priv(struct client *client)
{
  assert(client);
  return client->priv;
}

void client_set_priv(struct client *client, void *priv)
{
  assert(client);
  client->priv = priv;
}

void event_init(struct event *event, uint8_t type, event_fn callback)
{
  event->type = type;
  event->listener = NULL;
  event->callback = callback;
  event->user_callback = NULL;
  event->user_context = NULL;
}

struct set_callback_context {
  uint8_t event_type;
  uint8_t report_event;
  qmidev_response_fn callback;
  void *context;
};

static void trivial_set_callback_done(struct qmidev *qmidev,
                                      void *data)
{
  struct set_callback_context *context = data;
  context->callback(qmidev, context->context, 0);
  g_slice_free(struct set_callback_context, context);
}

int set_event_report_request(void *data,
                             struct qmimsg *message)
{
  struct set_callback_context *context = data;
  return qmimsg_tlv_add(message,
                        context->event_type,
                        sizeof(context->report_event),
                        &context->report_event);
}

void set_event_report_response(struct qmidev *qmidev,
                               void *data,
                               int result,
                               struct qmimsg *message)
{
  struct set_callback_context *context = data;

  /* nothing useful in message */
  message = message;

  context->callback(qmidev, context->context, result);
  g_slice_free(struct set_callback_context, context);
}

int qmidev_set_callback(struct qmidev *qmidev,
                        struct client *client,
                        struct event *event,
                        void *user_callback,
                        void *user_context,
                        qmidev_response_fn caller_callback,
                        void *caller_context)
{
  struct set_callback_context *context;
  int active_before, active_after;
  int result;

  active_before = !!event->user_callback;
  active_after = !!user_callback;

  context = g_slice_new(struct set_callback_context);
  context->event_type = event->type;
  context->report_event = active_after ? 0x01 : 0x00;
  context->callback = caller_callback;
  context->context = caller_context;

  event->user_callback = user_callback;
  event->user_context = user_context;

  if (active_before == active_after) {
    qmidev_call_later(qmidev, &trivial_set_callback_done, context);
    return 0;
  }

  if (active_after)
    client->events = g_list_prepend(client->events, event);
  else
    client->events = g_list_remove(client->events, event);

  result = qmidev_client_request(qmidev, client,
                                 client->service->event_msg_id,
                                 &set_event_report_request, context,
                                 &set_event_report_response, context);
  if (result)
    g_slice_free(struct set_callback_context, context);
  return result;
}


struct connect_context {
  GQueue *pending_services;

  qmidev_response_fn caller_callback;
  void *caller_context;
};

static void connect_next_service(struct qmidev *qmidev,
                                 struct connect_context *context);

static void got_client(struct qmidev *qmidev,
                       void *data,
                       int status,
                       struct client *client)
{
  struct connect_context *context = data;

  if (!status) {
    qmidev->clients = g_list_prepend(qmidev->clients, client);
  }

  connect_next_service(qmidev, context);
}

static void connect_next_service(struct qmidev *qmidev,
                                 struct connect_context *context)
{
  struct service *next_service;
  int result;

  assert(qmidev);
  assert(context);

  if (!g_queue_is_empty(context->pending_services)) {
    next_service = g_queue_pop_head(context->pending_services);
    result = qmidev_get_client(qmidev, next_service, &got_client, context);
    if (result)
      context->caller_callback(qmidev, context->caller_context, result);
  } else {
    context->caller_callback(qmidev, context->caller_context, 0);
    g_slice_free(struct connect_context, context);
  }
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

  qmidev->clients = g_list_prepend(qmidev->clients, make_ctl_client(qmidev));

  struct connect_context *context = g_slice_new(struct connect_context);
  context->caller_callback = caller_callback;
  context->caller_context = caller_context;
  context->pending_services = g_queue_new();

  g_queue_push_tail(context->pending_services, &qmidms_service);

  connect_next_service(qmidev, context);

  return 0;
}

struct disconnect_context {
  qmidev_response_fn caller_callback;
  void *caller_context;
};

static void disconnected_services(struct qmidev *qmidev,
                                  struct disconnect_context *context)
{
  int result = dev_close(qmidev->dev);
  qmidev->connected = 0;
  context->caller_callback(qmidev, context->caller_context, result);
  g_slice_free(struct disconnect_context, context);
}

static void disconnect_next_service(struct qmidev *qmidev,
                                    struct disconnect_context *context);

static void released_client(struct qmidev *qmidev,
                            void *data,
                            int status)
{
  if (status) {
    /* TODO: What do we do if release fails? */
  }

  disconnect_next_service(qmidev, data);
}

static void disconnect_next_service(struct qmidev *qmidev,
                                    struct disconnect_context *context)
{
  struct client *next_client;
  int result;

  if (!qmidev->clients) {
    disconnected_services(qmidev, context);
    return;
  }

  next_client = qmidev->clients->data;
  qmidev->clients = g_list_remove(qmidev->clients, next_client);
  if (!next_client->client_id) {
    /* This is the CTL client.  Free it by hand and try the next one. */
    g_slice_free(struct client, next_client);
    disconnect_next_service(qmidev, context);
    return;
  }

  result = qmidev_release_client(qmidev, next_client,
                                 &released_client, context);
  if (result)
    context->caller_callback(qmidev, context->caller_context, result);
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

  disconnect_next_service(qmidev, context);

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
  if (result)
    return;
  if (call_listeners(qmidev, msg) && qmidev_log_unhandled_messages) {
    fprintf(stderr, "Unhandled message received:\n");
    qmimsg_print(msg);
  }
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




/**
 * Starts listening for QMI messages matching certain criteria.
 *
 * @qmidev: the QMI device
 * @criteria: the listen criteria (see struct listen_criteria)
 * @callback: the callback to call when a matching message is received
 * @context: the context with which to call the callback
 *
 * Returns a pointer to a struct listener that can be passed to
 * qmidev_cancel_listen to stop listening.
 */

struct listener *qmidev_listen(struct qmidev *qmidev,
                               struct listen_criteria *criteria,
                               qmidev_listen_callback_fn callback,
                               void *context);

/**
 * Stops listening for QMI messages matching certain criteria.
 *
 * @qmidev: the QMI device
 * @listener: the listener returned by qmidev_listen
 */
void qmidev_cancel_listen(struct qmidev *qmidev, struct listener *listener);


