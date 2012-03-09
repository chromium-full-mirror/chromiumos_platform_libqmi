# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS += -Wall -Wextra -Werror -fPIC -I ../include

all: libqmi.so

LIBQMI_SRCS = dev.c file.c list.c mock.c poller.c qmictl.c qmidev.c qmimsg.c \
	qrb.c util.c
TESTS = list_unittest qmidev_unittest

LIBQMI_OBJS = $(LIBQMI_SRCS:.c=.o)
libqmi.so: $(LIBQMI_OBJS)
	$(CC) $(LDFLAGS) -Wl,-soname,libqmi.so -shared -o $@ $^

test: $(TESTS)
	./list_unittest
	./qmidev_unittest

list_unittest: $(LIBQMI_OBJS) list_unittest.o
	$(CC) $(LDFLAGS) -o $@ $^

qmidev_unittest: $(LIBQMI_OBJS) qmidev_unittest.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o libqmi.so $(TESTS)
