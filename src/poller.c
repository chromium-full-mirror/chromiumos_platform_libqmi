/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "poller.h"

#include <assert.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "util.h"

/* Based on https://github.com/elly/elib/blob/master/reactor.c */

struct poller {
  int epoll_fd;
  int num_polled;
};

struct polled {
  struct poller *poller;
  int fd;
  void *priv;
  void (*read)(struct polled *polled);
  void (*write)(struct polled *polled);
  void (*close)(struct polled *polled);
};

struct poller *poller_new(void)
{
  struct poller *poller;
  int epoll_fd;

  epoll_fd = epoll_create1(0);
  if (epoll_fd < 0)
    return NULL;
  poller = g_slice_new(struct poller);
  poller->epoll_fd = epoll_fd;
  poller->num_polled = 0;
  return poller;
}

void poller_free(struct poller *poller)
{
  assert(poller);
  assert(poller->num_polled == 0);
  close(poller->epoll_fd);
  g_slice_free(struct poller, poller);
}

int poller_fd(struct poller *poller)
{
  assert(poller);
  return poller->epoll_fd;
}

struct polled *poller_add(struct poller *poller, int fd)
{
  struct polled *polled;
  struct epoll_event event;

  assert(poller);

  polled = g_slice_new(struct polled);
  polled->poller = poller;
  polled->fd = fd;
  polled->priv = NULL;
  polled->read = NULL;
  polled->write = NULL;
  polled->close = NULL;

  event.events = 0;
  event.data.ptr = polled;

  if (epoll_ctl(poller->epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
    g_slice_free(struct polled, polled);
    return NULL;
  }

  poller->num_polled++;

  return polled;
}

static int poller_update(struct poller *poller, struct polled *polled)
{
  struct epoll_event event;

  assert(poller);
  assert(polled);
  assert(polled->poller == poller);

  event.events = 0;
  event.data.ptr = polled;

  if (polled->read)
    event.events |= EPOLLIN;
  if (polled->write)
    event.events |= EPOLLOUT;
  if (polled->close)
    event.events |= EPOLLRDHUP;

  return epoll_ctl(poller->epoll_fd, EPOLL_CTL_MOD, polled->fd, &event);
}

static int poller_remove(struct poller *poller, struct polled *polled)
{
  int result;

  assert(poller);
  assert(polled);
  assert(polled->poller == poller);

  result = epoll_ctl(poller->epoll_fd, EPOLL_CTL_DEL, polled->fd, NULL);
  if (result)
    return result;

  g_slice_free(struct polled, polled);
  poller->num_polled--;

  return 0;
}

int poller_poll(struct poller *poller)
{
  struct epoll_event events[16];
  struct polled *polled;
  int n, i;

  n = epoll_wait(poller->epoll_fd,
                 events, sizeof(events) / sizeof(events[0]),
                 0);
  if (n < 0)
    return n;
  for (i = 0; i < n; i++) {
    polled = events[i].data.ptr;
    if (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
      if (polled->close)
        (polled->close)(polled);
      poller_remove(poller, polled);
    } else if ((events[i].events & EPOLLIN) && polled->read) {
      (polled->read)(polled);
    } else if ((events[i].events & EPOLLOUT) && polled->write) {
      (polled->write)(polled);
    }
  }
  return 0;
}

void *polled_priv(struct polled *polled)
{
  assert(polled);
  return polled->priv;
}

void polled_set_priv(struct polled *polled, void *priv)
{
  assert(polled);
  polled->priv = priv;
}

void polled_set_read(struct polled *polled,
         void (*read)(struct polled *polled))
{
  assert(polled);
  polled->read = read;
}

void polled_set_write(struct polled *polled,
          void (*write)(struct polled *polled))
{
  assert(polled);
  polled->write = write;
}

void polled_set_close(struct polled *polled,
          void (*close)(struct polled *polled))
{
  assert(polled);
  polled->close = close;
}

int polled_update(struct polled *polled)
{
  assert(polled);
  return poller_update(polled->poller, polled);
}

int polled_delete(struct polled *polled)
{
  assert(polled);
  return poller_remove(polled->poller, polled);
}
