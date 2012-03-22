/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>

#include <libqmi.h>

static void disconnected(struct qmidev *qmidev, void *data, int status)
{
  int *done = data;

  /* unused */
  qmidev = qmidev;

  assert(status == 0);
  *done = 1;
}

static void got_ids(struct qmidev *qmidev, void *data, int status,
                    const char *esn, const char *imei, const char *meid)
{
  assert(status == 0);
  printf("ESN: %s\nIMEI: %s\nMEID: %s\n", esn, imei, meid);
  int result = qmidev_disconnect(qmidev, &disconnected, data);
  assert(result == 0);
}

static void connected(struct qmidev *qmidev, void *data, int status)
{
  assert(status == 0);
  int result = qmidev_get_ids(qmidev, &got_ids, data);
  assert(result == 0);
}

int main(void)
{
  int done = 0;
  int result;

  struct qmidev *qmidev = qmidev_new_file("/dev/cdc-wdm0");
  assert(qmidev);

  result = qmidev_connect(qmidev, &connected, &done);
  assert(result == 0);

  while (!done) {
    result = qmidev_process(qmidev);
    assert(result == 0);
  }

  return 0;
}
