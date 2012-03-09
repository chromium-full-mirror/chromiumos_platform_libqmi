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

#include <stddef.h>

struct qmidev;
struct qmimsg;
struct qrb;
struct callback;

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
 * Sends a QMI message.
 * Returns zero on success, errno on failure.
 */
int qmidev_send(struct qmidev *qmidev, struct qmimsg *msg);

/**
 * Listens for incoming messages matching a qrb.
 * Returns zero on success, errno on failure.
 */
int qmidev_listen(struct qmidev *qmidev, struct qrb *qrb);

typedef void (*callback_fn)(struct qmidev *qmidev, void *context);

/**
 * Calls a callback from qmidev_process instead of the current call stack.
 */
void qmidev_call(struct qmidev *qmidev, callback_fn func, void *context);

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
