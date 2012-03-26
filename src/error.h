/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * error: internal errors (eventually to move to libqmi.h)
 */

#ifndef LIBQMI_ERROR_H
#define LIBQMI_ERROR_H

enum {
  QMI_ERR_FRAMING_INVALID = 1,
  QMI_ERR_HEADER_INVALID,
  QMI_ERR_TLV_NOT_FOUND,
  QMI_ERR_MESSAGE_INVALID,
  QMI_ERR_SERVICE_UNAVAILABLE,
  QMI_ERR_TOO_LONG,
  QMI_ERR_RESPONSE_TIMEOUT,

  QMI_ERR_NOT_CONNECTED,
  QMI_ERR_ALREADY_CONNECTED,

  QMI_ERR_INTERNAL,

  QMI_ERR_DEVICE_OFFSET,
};

#endif /* LIBQMI_ERROR_H */
