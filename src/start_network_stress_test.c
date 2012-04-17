/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>

#include <libqmi.h>

struct context {
  uint32_t session_id;
};

static void stopped(struct qmidev *qmidev, void *data, int status);
static void started(struct qmidev *qmidev, void *data, int status,
                    qmidev_session_id session_id, uint16_t end);

static void start(struct qmidev *qmidev, struct context *context)
{
  int result;

  assert(!context->session_id);
  result = qmidev_start_network(qmidev, &started, context);
  assert(result == 0);
}

static void stop(struct qmidev *qmidev, struct context *context)
{
  int result;

  assert(context->session_id);
  result = qmidev_stop_network(qmidev, context->session_id,
                               &stopped, context);
  assert(result == 0);
}

static void started(struct qmidev *qmidev, void *data, int status,
                    qmidev_session_id session_id, uint16_t end)
{
  struct context *context = data;
  printf("started: status = %d session_id = 0x%08x call_end_reason = %d\n",
         status, session_id, end);
  assert(status == 0);
  context->session_id = session_id;
  stop(qmidev, context);

  /* unused */
  qmidev = qmidev;
}

static void stopped(struct qmidev *qmidev, void *data, int status)
{
  struct context *context = data;
  printf("stopped: status = %d\n", status);
  assert(status == 0);
  context->session_id = 0;
  start(qmidev, context);
}

static void packet_status_changed(struct qmidev *qmidev,
                                  void *data,
                                  int status,
                                  qmidev_packet_status state)
{
  printf("packet_status_changed: status = %d state = %d\n", status, state);
  qmidev = qmidev;
  data = data;
}

static void connected(struct qmidev *qmidev, void *data, int status)
{
  int result;

  assert(status == 0);
  result = qmidev_set_packet_status_callback(qmidev,
                                             &packet_status_changed, data,
                                             NULL, NULL);
  assert(result == 0);
  start(qmidev, data);
}

int main(void)
{
  struct qmidev *qmidev;
  struct context context;
  int result;

  context.session_id = 0;

  qmidev = qmidev_new_file("/dev/cdc-wdm0");
  assert(qmidev);

  result = qmidev_connect(qmidev, &connected, &context);
  assert(result == 0);

  while (1) {
    result = qmidev_process(qmidev);
    assert(result == 0);
  }

  return 0;
}
