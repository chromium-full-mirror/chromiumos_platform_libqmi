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

enum {
  QMI_FLAG_CTL_RESPONSE   = 0x01,
  QMI_FLAG_CTL_INDICATION = 0x02
};

enum {
  QMI_FLAG_SVC_COMPOUND   = 0x01,
  QMI_FLAG_SVC_RESPONSE   = 0x02,
  QMI_FLAG_SVC_INDICATION = 0x04
};

enum {
  QMI_CID_NONE = 0x00,
  QMI_CID_BROADCAST = 0xFF
};

enum {
  QMI_TLV_VALUE = 0x01
};

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

enum {
  QMICTL_MSG_SET_INSTANCE_ID = 0x0020,
  QMICTL_MSG_GET_VERSION_INFO,
  QMICTL_MSG_GET_CLIENT_ID,
  QMICTL_MSG_RELEASE_CLIENT_ID,
  QMICTL_MSG_REVOKE_CLIENT_ID,
  QMICTL_MSG_INVALID_CLIENT_ID,
  QMICTL_MSG_SET_DATA_FORMAT,
  QMICTL_MSG_SYNC,
  QMICTL_MSG_EVENT,
  QMICTL_MSG_SET_POWER_SAVE_CONFIG,
  QMICTL_MSG_SET_POWER_SAVE_MODE,
  QMICTL_MSG_GET_POWER_SAVE_MODE
};

enum {
  QMIDMS_MSG_EVENT              = 0x0001,
  QMIDMS_MSG_GET_IDS            = 0x0025,
  QMIDMS_MSG_GET_OPERATING_MODE = 0x002D,
  QMIDMS_MSG_SET_OPERATING_MODE = 0x002E
};

enum {
  QMIDMS_EVENT_POWER_STATE = 0x10,
  QMIDMS_EVENT_BATTERY_LEVEL,
  QMIDMS_EVENT_PIN_STATUS,
  QMIDMS_EVENT_ACTIVATION_STATE,
  QMIDMS_EVENT_OPERATING_MODE,
  QMIDMS_EVENT_UIM_STATE,
  QMIDMS_EVENT_WIRELESS_DISABLE_STATE
};


/**
 * Gets the result from a TLV message.
 *
 * Returns 0 if the result code indicates success, an appropriate error
 * code if the result code indicates failure, or QMI_ERR_MESSAGE_INVALID
 * if the message does not contain a result code TLV.
 */
int qmi_result(struct qmimsg *message);

int qmi_tlv_strdup(struct qmimsg *message,
                   uint8_t type,
                   uint16_t *size_out,
                   char **value_out);

#endif /* LIBQMI_QMIDEV_H */
