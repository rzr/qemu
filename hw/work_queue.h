#ifndef _QEMU_WORK_QUEUE_H
#define _QEMU_WORK_QUEUE_H

#include "qemu-common.h"
#include "qemu/queue.h"
#include "qemu/thread.h"

struct work_queue_item;

typedef void (*work_queue_func)(struct work_queue_item */*wq_item*/);

struct work_queue_item
{
    QTAILQ_ENTRY(work_queue_item) entry;

    work_queue_func func;
};

struct work_queue
{
    QemuThread thread;
    QemuMutex mutex;
    QemuCond add_cond;
    QemuCond wait_cond;

    QTAILQ_HEAD(, work_queue_item) items;

    int num_items;

    bool destroying;
};

void work_queue_item_init(struct work_queue_item *wq_item,
                          work_queue_func func);

struct work_queue *work_queue_create(void);

void work_queue_add_item(struct work_queue *wq,
                         struct work_queue_item *wq_item);

void work_queue_wait(struct work_queue *wq);

void work_queue_destroy(struct work_queue *wq);

#endif
