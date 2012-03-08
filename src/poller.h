/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * poller: an abstraction around epoll, shamelessly stolen from ellyjones.
 *
 * Based on https://github.com/elly/elib/blob/master/include/elib/reactor.h
 */

#ifndef LIBQMI_POLLER_H
#define LIBQMI_POLLER_H

struct poller *poller_new(void);
void poller_free(struct poller *poller);
int poller_fd(struct poller *poller);
struct polled *poller_add(struct poller *poller, int fd);
int poller_poll(struct poller *poller);

void *polled_priv(struct polled *polled);
void polled_set_priv(struct polled *polled, void *priv);
void polled_set_read(struct polled *polled, void (*read)(struct polled *));
void polled_set_write(struct polled *polled, void (*write)(struct polled *));
void polled_set_close(struct polled *polled, void (*close)(struct polled *));
int polled_update(struct polled *polled);
int polled_delete(struct polled *polled);

#endif /* LIBQMI_POLLER_H */
