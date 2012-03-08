/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * utils: various utility functions.
 */

#ifndef LIBQMI_UTIL_H
#define LIBQMI_UTIL_H

#include <stddef.h>

/**
 * Like malloc, but never fails.  (Aborts the program if malloc fails.)
 */
void *xmalloc(size_t size);

/**
 * Like realloc, but never fails.  (Aborts the program if realloc fails.)
 */
void *xrealloc(void *p, size_t s);

/**
 * Like strdup, but definitely exists.
 */
char *xstrdup(const char *s);

/**
 * xfree, to accompany xmalloc.
 */
void xfree(void *p);

/**
 * Prints len bytes at buf to stderr in hex.
 */
void hexdump(const char *buf, size_t len);

#endif /* LIBQMI_UTIL_H */
