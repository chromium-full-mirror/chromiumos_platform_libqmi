/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmidev: internal interface to the top-level QMI device object.
 *
 * (Also see ../include/qmidev.h for the public interface.)
 */

#ifndef LIBQMI_QMIDEV_H
#define LIBQMI_QMIDEV_H

#include <libqmi.h>

#include <stddef.h>
#include <stdint.h>

struct qmidev;
struct qmimsg;
struct callback;

struct client;

typedef void (*client_create_fn)(struct qmidev *qmidev,
                                 struct client *client);

typedef void (*client_destroy_fn)(struct qmidev *qmidev,
                                  struct client *client);

typedef int (*request_fn)(void *context,
                          struct qmimsg *message);

typedef void (*response_fn)(struct qmidev *qmidev,
                            void *context,
                            int status,
                            struct qmimsg *message);

typedef void (*event_fn)(struct qmidev *qmidev,
                         void *user_callback,
                         void *user_context,
                         uint16_t length,
                         void *value);

struct event {
  uint8_t type;
  struct listener *listener;
  event_fn callback;
  void *user_callback;
  void *user_context;
};

struct service {
  uint8_t service_id;
  uint16_t event_msg_id;

  client_create_fn create_fn;
  client_destroy_fn destroy_fn;
};

#define INIT_SERVICE(id, event_msg_id) { (id), (event_msg_id), NULL, NULL }

void *client_priv(struct client *client);

void client_set_priv(struct client *client, void *priv);

/**
 * Fills in a struct event.
 *
 * @event: the struct event to fill in
 * @type: the type of the TLV element representing the event
 * @callback: the callback used to call the user's callback when the event
 *            happens
 */
void event_init(struct event *event, uint8_t type, event_fn callback);

/** Callback type for qmidev_get_client */
typedef void (*qmidev_client_fn)(struct qmidev *qmidev,
                                 void *context,
                                 int status,
                                 struct client *client);

/**
 * Allocates a client for a particular service on the card.
 */
int qmidev_get_client(struct qmidev *qmidev,
                      struct service *service,
                      qmidev_client_fn callback,
                      void *context);


/**
 * Releases an allocated client.
 */
int qmidev_release_client(struct qmidev *qmidev,
                          struct client *client,
                          qmidev_response_fn callback,
                          void *context);

/**
 * Returns the default client for a given service.
 */
struct client *qmidev_default_client(struct qmidev *qmidev,
                                     struct service *service);

/*
 * General request structure:
 *
 * Most requests will have four declarations:
 *
 * 1. a context struct to be passed to the response handler.  This stores the
 *    callback and context passed to the main request function, so the
 *    response handler can call the callback after extracting any returned
 *    data from the response message.
 *
 * 2. (optional) a make_request function.  If the request requires any TLVs to
 *    be included in the request message, this function takes a QMI message
 *    and a context pointer and adds the data to the message.  (This is a
 *    separate function because it makes things neater for requests that don't
 *    have any parameter -- a NULL make_request function simply isn't called.)
 *
 * 3. a handle_response function.  This is called when a response to the
 *    request is received, and is responsible for getting any returned data
 *    from the response message, calling the original callback, and freeing
 *    the context.
 *
 * 4. the main request function.  This allocates a context and calls
 *    qmidev_request with pointers to the request and response functions and
 *    associated contexts.
 */

/**
 * Makes a request on a particular client (as opposed to the default one) of a
 * service.
 *
 * @qmidev: the QMI device
 * @client: the client on which to make the request
 * @msg_id: the message ID of the request and response messages
 * @make_request_fn: a function to be called to fill in the request message,
 *                   if it requires a payload (otherwise NULL).  Will be
 *                   called before qmidev_client_request returns.
 * @request_context: the context for make_request_fn
 * @handle_response_fn: the function to call when the response is received
 * @response_context: the context for handle_response_fn
 */
int qmidev_client_request(struct qmidev *qmidev,
                          struct client *client,
                          uint16_t msg_id,
                          request_fn make_request_fn,
                          void *request_context,
                          response_fn handle_response_fn,
                          void *response_context);

/**
 * Makes a request on the default client of a service.
 *
 * Arguments are otherwise the same as qmidev_client_request.
 */
int qmidev_request(struct qmidev *qmidev,
                   struct service *service,
                   uint16_t msg_id,
                   request_fn make_request_fn,
                   void *request_context,
                   response_fn handle_response_fn,
                   void *response_context);

/**
 * Makes a void request on the default client of a service.
 *
 * Immediately calls the response function without doing anything with the
 * response message besides getting the result code.
 *
 * Arguments are otherwise the same as qmidev_request.
 */
int qmidev_void_request(struct qmidev *qmidev,
                        struct service *service,
                        uint16_t msg_id,
                        request_fn make_request_fn,
                        void *request_context,
                        qmidev_response_fn handle_response_fn,
                        void *response_context);

/**
 * Sets or clears a callback.
 *
 * @qmidev: the QMI device
 * @client: the client on which to set/clear the callback
 * @event: the event structure representing the event whose callback we are
 *         setting or clearing
 * @new_callback: the new callback to set, or NULL if clearing
 * @new_context: the context for new_callback
 * @caller_callback: the callback to call once new_callback is set
 * @caller_context: the context for caller_callback
 */
int qmidev_set_callback(struct qmidev *qmidev,
                        struct client *client,
                        struct event *event,
                        void *new_callback,
                        void *new_context,
                        qmidev_response_fn caller_callback,
                        void *caller_context);

/* TODO: These should be replaced by qmidev_poll. */

/**
 * Sets a file descriptor belonging to the dev underlying this qmidev.
 * The qmidev will read from the dev when this filehandle is ready to read.
 * Returns zero on success, errno on failure.
 */
int qmidev_set_dev_fd(struct qmidev *qmidev, int fd);

/**
 * Clears the file descriptor set by qmidev_set_dev_fd.
 * Returns zero on success, errno on failure.
 */
int qmidev_clear_dev_fd(struct qmidev *qmidev);

/**
 * Adds a file descriptor to the poller in this qmidev.
 * Returns the new struct polled * on success, NULL on failure.
 */
struct polled *qmidev_poll(struct qmidev *qmidev, int fd);

/**
 * Callback type for QMI requests made with qmidev_send_request.
 *
 * @resp_msg: The response message received, or NULL if none.
 */
typedef void (*qmidev_request_fn)(struct qmidev *qmidev,
                                  void *context,
                                  int status,
                                  struct qmimsg *resp_msg);

/**
 * Sends a QMI message.
 * Returns zero on success, errno on failure.
 */
int qmidev_send(struct qmidev *qmidev, struct qmimsg *msg);

typedef void (*callback_fn)(struct qmidev *qmidev, void *context);

/**
 * Calls a callback from qmidev_process instead of the current call stack.
 */
void qmidev_call_later(struct qmidev *qmidev, callback_fn func,
                       void *context);

/* Mock qmidev functions: */

/* Create a new mock QMI device (backed by pipes instead of a real file.) */
struct qmidev *qmidev_new_mock(void);

/* Check if the mock device os open. */
int qmidev_mock_is_open(struct qmidev *qmidev);

/* Inject a message "received from" the mock device. */
int qmidev_mock_inject(struct qmidev *qmidev, const void *buf, size_t len);

/* Extract a message "sent to" the mock device.
 *
 * @len: A pointer to the length of buf; it will be replaced with the size of
 *       the message read.  Cannot be NULL.
 *
 * Returns 0 on success,
 * ENOSPC if len is too small for the next message.
 */
int qmidev_mock_extract(struct qmidev *qmidev, void *buf, size_t *len);

#endif /* LIBQMI_QMIDEV_H */
