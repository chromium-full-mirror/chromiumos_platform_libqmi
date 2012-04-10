/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "qmi.h"

#include <assert.h>
#include <glib.h>
#include <stdint.h>

#include "error.h"
#include "qmimsg.h"

struct qmi_result {
  uint16_t status;
  uint16_t error;
};

/* 0x02 is the TLV for the result of any request.
   (grep "Result Code" [Gobi3000API]/Database/QMI/Entity.txt) */
#define QMI_TLV_RESULT_CODE ((uint8_t)0x02)

enum {
  QMI_STATUS_SUCCESS = 0x0000,
  QMI_STATUS_FAILURE = 0x0001
};

int qmi_result(struct qmimsg *message)
{
  struct qmi_result msg_result;
  int result;

  assert(message);

  result = qmimsg_tlv_get(message, QMI_TLV_RESULT_CODE,
                          sizeof(msg_result), &msg_result);
  if (result)
    return QMI_ERR_MESSAGE_INVALID;

  switch (msg_result.status) {
  case QMI_STATUS_SUCCESS:
    return 0;
  case QMI_STATUS_FAILURE:
    return QMI_ERR_DEVICE_OFFSET + msg_result.error;
  default:
    return QMI_ERR_MESSAGE_INVALID;
  }
}

int qmi_tlv_strdup(struct qmimsg *message,
                   uint8_t type,
                   uint16_t *length_out,
                   char **value_out)
{
  uint16_t length;
  char *value;
  int result;

  assert(message);
  assert(value_out);

  result = qmimsg_tlv_get_varlen(message, type, &length, NULL);
  if (result)
    return result;

  value = g_malloc(length + 1);
  result = qmimsg_tlv_get(message, type, length, value);
  assert(result == 0);

  value[length] = '\0';
  *value_out = value;
  if (length_out)
    *length_out = length;

  return 0;
}
