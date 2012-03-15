/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>

#include <libqmi.h>
#include "qmidev.h"
#include "qmictl.h"

struct context {
  int count;
  int done;
};

static void disconnected(struct qmidev *qmidev, void *data, int status)
{
  struct context *context = data;

  qmidev = qmidev;

  assert(status == 0);
  printf("disconnected\n");
  context->done = 1;
}

static void gotcid(struct qmidev *qmidev, void *context, int status,
                   uint8_t cid);

static void releasedcid(struct qmidev *qmidev, void *data, int status)
{
  struct context *context = data;
  int result;

  assert(status == 0);
  printf("releasedcid\n");
  if (--context->count > 0) {
    result = qmictl_get_cid(qmidev, 0x01, &gotcid, data);
    assert(result == 0);
  } else {
    result = qmidev_disconnect(qmidev, &disconnected, data);
    assert(result == 0);
  }
}

static void gotcid(struct qmidev *qmidev, void *data, int status,
                   uint8_t cid)
{
  int result;

  assert(status == 0);
  printf("gotcid(cid=0x%02x)\n", cid);
  result = qmictl_release_cid(qmidev, 0x01, cid, &releasedcid, data);
  assert(result == 0);
}

static void connected(struct qmidev *qmidev, void *data, int status)
{
  int result;

  assert(status == 0);
  printf("connected\n");
  result = qmictl_get_cid(qmidev, 0x01, &gotcid, data);
  assert(result == 0);
}

int main(void)
{
  struct context context;
  int result;

  struct qmidev *qmidev = qmidev_new_file("/dev/cdc-wdm0");
  assert(qmidev);

  context.count = 256;
  context.done = 0;

  result = qmidev_connect(qmidev, &connected, &context);
  assert(result == 0);

  while (!context.done) {
    result = qmidev_process(qmidev);
    assert(result == 0);
  }

  return 0;
}
