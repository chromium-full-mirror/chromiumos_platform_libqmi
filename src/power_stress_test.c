/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <libqmi.h>

struct context {
  qmidev_power_state last_state;
  int done;
};

static void set_power(struct qmidev *qmidev, void *data, int status)
{
  assert(status == 0);
  printf("set_power: status = %d\n", status);
  qmidev = qmidev;
  data = data;
}

static void set_next_state(struct qmidev *qmidev, struct context *context)
{
  qmidev_power_state next_state = 1 - context->last_state;
  int result = qmidev_set_power(qmidev, next_state, &set_power, context);
  assert(result == 0);
}

static void power_callback(struct qmidev *qmidev, void *data, int status,
                           qmidev_power_state state)
{
  struct context *context = data;

  assert(status == 0);
  printf("power_callback: status = %d state = %d\n", status, state);
  context->last_state = state;
  set_next_state(qmidev, context);
}

static void got_power(struct qmidev *qmidev, void *data, int status,
                      qmidev_power_state state)
{
  struct context *context = data;

  assert(status == 0);
  printf("got_power: status = %d state = %d\n", status, state);
  context->last_state = state;
  set_next_state(qmidev, context);
}

static void set_power_callback(struct qmidev *qmidev, void *data, int status)
{
  assert(status == 0);
  printf("set_power_callback: status %d\n", status);
  int result = qmidev_get_power(qmidev, &got_power, data);
  assert(result == 0);
}

static void connected(struct qmidev *qmidev, void *data, int status)
{
  assert(status == 0);
  int result = qmidev_set_power_callback(qmidev,
                                         &power_callback, data,
                                         &set_power_callback, data);
  assert(result == 0);
}

int main(void)
{
  struct context context;
  int result;

  context.done = 0;

  struct qmidev *qmidev = qmidev_new_file("/dev/cdc-wdm0");
  assert(qmidev);

  result = qmidev_connect(qmidev, &connected, &context);
  assert(result == 0);

  while (!context.done) {
    result = qmidev_process(qmidev);
    assert(result == 0);
  }

  return 0;
}
