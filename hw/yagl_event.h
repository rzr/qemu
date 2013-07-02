#ifndef _QEMU_YAGL_EVENT_H
#define _QEMU_YAGL_EVENT_H

#include "yagl_types.h"
#include "qemu-thread.h"

struct yagl_event
{
    bool manual_reset;
    volatile bool state;
    QemuMutex mutex;
    QemuCond cond;
};

void yagl_event_init(struct yagl_event *event,
                     bool manual_reset,
                     bool initial_state);

void yagl_event_cleanup(struct yagl_event *event);

void yagl_event_set(struct yagl_event *event);

void yagl_event_reset(struct yagl_event *event);

void yagl_event_wait(struct yagl_event *event);

bool yagl_event_is_set(struct yagl_event *event);

#endif
