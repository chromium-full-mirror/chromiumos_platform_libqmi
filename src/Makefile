# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS += -Wall -Wextra -Werror -fPIC -I ../include
LIBDIR = /usr/lib
SBINDIR = /usr/sbin

all: libqmi.so get_cid_stress_test

LIBQMI_SRCS = dev.c file.c list.c mock.c poller.c qmictl.c qmidev.c qmimsg.c \
	util.c
UNITTESTS = list_unittest qmidev_unittest

LIBQMI_OBJS = $(LIBQMI_SRCS:.c=.o)
LINK.c = $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

libqmi.so: $(LIBQMI_OBJS)
	$(CC) $(LDFLAGS) -Wl,-soname,libqmi.so -shared -o $@ $^

test: $(UNITTESTS)
	./list_unittest
	./qmidev_unittest

list_unittest: $(LIBQMI_OBJS) list_unittest.o
	$(LINK.c)

qmidev_unittest: $(LIBQMI_OBJS) qmidev_unittest.o
	$(LINK.c)

get_cid_stress_test: libqmi.so get_cid_stress_test.o
	$(LINK.c)

install:
	install -d $(DESTDIR)$(LIBDIR)
	install -m755 libqmi.so $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(SBINDIR)
	install -m755 get_cid_stress_test $(DESTDIR)$(SBINDIR)

clean:
	rm -f *.o libqmi.so $(UNITTESTS) get_cid_stress_test
