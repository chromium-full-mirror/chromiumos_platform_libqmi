/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hexdump(const char *buf, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++) {
    if (i > 0 && (i % 32 == 0))
      fprintf(stderr, "\n");
    fprintf(stderr, " %02x", buf[i]);
  }
  fprintf(stderr, "\n");
}
