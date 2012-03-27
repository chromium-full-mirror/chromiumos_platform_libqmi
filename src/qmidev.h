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

struct listener;

struct listen_criteria {
  uint8_t service;
  uint8_t client;
  uint16_t transaction;
};

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
 * Creates a new QMI message for a request.
 *
 * The service and message ID fields of the message will be filled from the
 * respective parameters.  The client ID is set to the client ID allocated
 * by the qmidev for this service, and the transaction ID is assigned
 * sequentially within that client.
 *
 * @service: The service to which the request will be sent
 * @req_msg_id: The message ID of the request
 */
int qmidev_make_request(struct qmidev *qmidev,
                        uint8_t service_id,
                        uint16_t req_msg_id,
                        struct qmimsg **message_out);

/**
 * Sends a request message created with qmidev_make_request, and listens for
 * a reply.
 *
 * @req_msg: The message returned from qmidev_make_request
 * @resp_msg_id: The expected message ID of the response
 * @callback: The callback to call when a response is received
 * @context: The context with which to call the callback
 */
int qmidev_send_request(struct qmidev *qmidev,
                        struct qmimsg *req_msg,
                        uint16_t resp_msg_id,
                        qmidev_request_fn callback,
                        void *context);

int qmidev_send_void_request(struct qmidev *qmidev,
                             struct qmimsg *req_msg,
                             uint16_t resp_msg_id,
                             qmidev_response_fn caller_callback,
                             void *caller_context);

/**
 * Sends a QMI message.
 * Returns zero on success, errno on failure.
 */
int qmidev_send(struct qmidev *qmidev, struct qmimsg *msg);

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
