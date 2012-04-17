# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CFLAGS += -Wall -Wextra -Werror -Wdeclaration-after-statement
LIBDIR = /usr/lib
SBINDIR = /usr/sbin

LIBQMI_SRCS = \
	dev.c \
	file.c \
	mock.c \
	poller.c \
	qmi.c \
	qmictl.c \
	qmidev.c \
	qmidms.c \
	qmimsg.c \
	qmiwds.c \
	util.c

UNITTESTS = \
	qmidev_unittest
TESTS = \
	connect_disconnect_test \
	get_cid_stress_test \
	power_stress_test \
	qmiinfo \
	start_network_stress_test

PC_CFLAGS := $(shell pkg-config --cflags glib-2.0)
CFLAGS += -fpic -I ../include $(PC_CFLAGS)
PC_LIBS := $(shell pkg-config --libs glib-2.0)
LDLIBS += $(PC_LIBS)

LIBQMI_OBJS = $(LIBQMI_SRCS:.c=.o)
LINK.c = $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

all: libqmi.so $(TESTS)

libqmi.so: LDFLAGS += -Wl,-soname,libqmi.so -shared
libqmi.so: $(LIBQMI_OBJS)
	$(LINK.c)

test: $(UNITTESTS)
	./qmidev_unittest

qmidev_unittest: $(LIBQMI_OBJS) qmidev_unittest.o
	$(LINK.c)

$(TESTS): LDLIBS += libqmi.so

connect_disconnect_test: connect_disconnect_test.o | libqmi.so
	$(LINK.c)

get_cid_stress_test: get_cid_stress_test.o | libqmi.so
	$(LINK.c)

power_stress_test: power_stress_test.o | libqmi.so
	$(LINK.c)

qmiinfo: qmiinfo.o | libqmi.so
	$(LINK.c)

start_network_stress_test: start_network_stress_test.o | libqmi.so
	$(LINK.c)

install:
	install -d $(DESTDIR)$(LIBDIR)
	install -m755 libqmi.so $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(SBINDIR)
	install -m755 connect_disconnect_test $(DESTDIR)$(SBINDIR)
	install -m755 get_cid_stress_test $(DESTDIR)$(SBINDIR)
	install -m755 power_stress_test $(DESTDIR)$(SBINDIR)
	install -m755 qmiinfo $(DESTDIR)$(SBINDIR)
	install -m755 start_network_stress_test $(DESTDIR)$(SBINDIR)

clean:
	rm -f *.o libqmi.so $(UNITTESTS) $(TESTS)
