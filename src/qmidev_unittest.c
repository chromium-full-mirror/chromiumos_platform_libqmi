/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <unistd.h>

#include <libqmi.h>
#include "qmidev.h"

static void hello(struct qmidev *qmidev, void *context)
{
  qmidev = qmidev;
  context = context;

  exit(0);
}

int main(void)
{
  struct qmidev *qmidev = qmidev_new_file("/dev/null");
  qmidev_call(qmidev, hello, NULL);
  alarm(1);
  qmidev_process(qmidev);
  return 0;
}
