/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmictl: QMI calls on the CTL (control) service.
 *
 * (See qmictl.c for sources.)
 */

#ifndef LIBQMI_QMICTL_H
#define LIBQMI_QMICTL_H

#include <stdint.h>

#include <libqmi.h>

/**
 * Callback type for qmictl_get_cid.
 *
 * @cid: The allocated client ID, if successful.
 */
typedef void (*qmictl_get_cid_response_fn)(struct qmidev *qmidev,
                                           void *context,
                                           int status,
                                           uint8_t cid);
/**
 * Allocate a client ID on the device.
 *
 * @service: The QMI service we're requesting a client ID for
 */
int qmictl_get_cid(struct qmidev *qmidev, uint8_t service,
                   qmictl_get_cid_response_fn cb, void *ctx);

/**
 * Release an allocated client ID on the device.
 *
 * @service: The QMI service of the client we're releasing
 * @client: The client ID of teh client we're releasing
 */
int qmictl_release_cid(struct qmidev *qmidev, uint8_t service, uint8_t client,
                       qmidev_response_fn cb, void *ctx);

#endif /* LIBQMI_QMICTL_H */
