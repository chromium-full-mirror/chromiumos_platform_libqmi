/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * mock: device implementation for mock files.
 *
 * Provides "inject" and "extract" functions to simulate messages from and to
 * the device.
 *
 * (Also see ../include/qmidev-mock.h for the public interface.)
 */

#ifndef LIBQMI_MOCK_H
#define LIBQMI_MOCK_H

struct dev;

/**
 * Creates a new mock device.
 */
struct dev *mock_new(void);

#endif /* LIBQMI_MOCK_H */
