/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "qmi.h"

#include <assert.h>
#include <stdint.h>

#include "qmimsg.h"

struct qmi_result {
  uint16_t status;
  uint16_t error;
};

/* 0x02 is the TLV for the result of any request.
   (grep "Result Code" [Gobi3000API]/Database/QMI/Entity.txt) */
#define QMI_TLV_RESULT_CODE ((uint8_t)0x02)

int qmi_result(struct qmimsg *message,
               uint16_t *status_out, uint16_t *error_out)
{
  assert(message);
  assert(status_out);
  assert(error_out);

  struct qmi_result qmi_result;

  int result = qmimsg_tlv_get(message, QMI_TLV_RESULT_CODE,
                              sizeof(qmi_result), &qmi_result);
  if (result)
    return result;

  *status_out = qmi_result.status;
  *error_out = qmi_result.error;

  return 0;
}
