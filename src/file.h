/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * file: device implementation for actual files.
 */

#ifndef LIBQMI_FILE_H
#define LIBQMI_FILE_H

struct qmidev;
struct dev;

/**
 * Creates a new device backed by a file.
 *
 * @qmidev: The QMI device that this device is being created to serve.
 * @file: The filename of the actual file to access (e.g. "/dev/cdc-wdm0").
 */
struct dev *file_new(struct qmidev *qmidev, const char *path);

#endif /* LIBQMI_FILE_H */
