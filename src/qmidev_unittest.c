/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <libqmi.h>
#include "qmidev.h"

static struct qmidev *mock_qmidev;
static int in_process;
static int connected_called;
static int disconnected_called;

void connected(struct qmidev *qmidev, void *context, int status)
{
  assert(qmidev == mock_qmidev);
  assert(context == NULL);
  assert(status == 0);
  assert(in_process);

  connected_called = 1;
}

void disconnected(struct qmidev *qmidev, void *context, int status)
{
  assert(qmidev == mock_qmidev);
  assert(context == NULL);
  assert(status == 0);
  assert(in_process);

  disconnected_called = 1;
}

void process(void)
{
  in_process = 1;
  qmidev_process(mock_qmidev);
  in_process = 0;
}

int main(void)
{
  int result;

  mock_qmidev = qmidev_new_mock();

  result = qmidev_connect(mock_qmidev, &connected, NULL);
  assert(!connected_called);
  assert(result == 0);
  process();
  assert(connected_called);

  result = qmidev_disconnect(mock_qmidev, &disconnected, NULL);
  assert(!disconnected_called);
  assert(result == 0);
  process();
  assert(disconnected_called);

  return 0;
}
