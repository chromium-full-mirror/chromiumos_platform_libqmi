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

#define QMI_CID_NONE ((uint8_t)0x00)
#define QMI_CID_BROADCAST ((uint8_t)0xFF)

enum {
  /** Control service */
  QMI_SVC_CTL = 0x00,
  /** Wireless Data Service */
  QMI_SVC_WDS = 0x01,
  /** Device Management Service */
  QMI_SVC_DMS = 0x02,
  /** Network Access Service */
  QMI_SVC_NAS = 0x03,
  /** Quality Of Service service */
  QMI_SVC_QOS = 0x04,
  /** Wireless Messaging Service */
  QMI_SVC_WMS = 0x05,
  /** Position Determination Service */
  QMI_SVC_PDS = 0x06,
  /** Authentication service */
  QMI_SVC_AUTH = 0x07,

  /** Voice service */
  QMI_SVC_VOICE = 0x09,

  /** Card Application Toolkit service */
  QMI_SVC_CAT = 0xE0,
  /** Remote Management Service */
  QMI_SVC_RMS = 0xE1,
  /** Open Mobile Alliance device management service */
  QMI_SVC_OMA = 0xE2
};

/**
 * Gets the result (status and error) from a TLV message.
 */
int qmi_result(struct qmimsg *message,
               uint16_t *status_out, uint16_t *error_out);

int qmi_tlv_strdup(struct qmimsg *message,
                   uint8_t type,
                   uint16_t *size_out,
                   char **value_out);

#endif /* LIBQMI_QMIDEV_H */
