/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmi: General QMI functions
 */

#ifndef LIBQMI_QMI_H
#define LIBQMI_QMI_H

#include <stdint.h>

struct qmimsg;

/**
 * Gets the result (status and error) from a TLV message.
 */
int qmi_result(struct qmimsg *message,
               uint16_t *status_out, uint16_t *error_out);

#endif /* LIBQMI_QMIDEV_H */
