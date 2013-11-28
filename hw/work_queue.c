#include "work_queue.h"

static void *work_queue_run(void *arg)
{
    struct work_queue *wq = arg;

    qemu_mutex_lock(&wq->mutex);

    while (true) {
        struct work_queue_item *wq_item;

        while (wq->num_items == 0) {
            if (wq->destroying) {
                goto out;
            }

            qemu_cond_signal(&wq->wait_cond);
            qemu_cond_wait(&wq->add_cond, &wq->mutex);
        }

        wq_item = QTAILQ_FIRST(&wq->items);
        QTAILQ_REMOVE(&wq->items, wq_item, entry);

        qemu_mutex_unlock(&wq->mutex);

        wq_item->func(wq_item);

        qemu_mutex_lock(&wq->mutex);

        --wq->num_items;
    }

out:
    qemu_mutex_unlock(&wq->mutex);

    return NULL;
}

void work_queue_item_init(struct work_queue_item *wq_item,
                          work_queue_func func)
{
    memset(wq_item, 0, sizeof(*wq_item));

    wq_item->func = func;
}

struct work_queue *work_queue_create(void)
{
    struct work_queue *wq;

    wq = g_malloc0(sizeof(*wq));

    qemu_mutex_init(&wq->mutex);
    qemu_cond_init(&wq->add_cond);
    qemu_cond_init(&wq->wait_cond);

    QTAILQ_INIT(&wq->items);

    qemu_thread_create(&wq->thread,
                       work_queue_run,
                       wq,
                       QEMU_THREAD_JOINABLE);

    return wq;
}

void work_queue_add_item(struct work_queue *wq,
                         struct work_queue_item *wq_item)
{
    qemu_mutex_lock(&wq->mutex);

    QTAILQ_INSERT_TAIL(&wq->items, wq_item, entry);
    ++wq->num_items;

    qemu_mutex_unlock(&wq->mutex);

    qemu_cond_signal(&wq->add_cond);
}

void work_queue_wait(struct work_queue *wq)
{
    if (wq->num_items == 0) {
        return;
    }

    qemu_mutex_lock(&wq->mutex);

    while (wq->num_items > 0) {
        qemu_cond_wait(&wq->wait_cond, &wq->mutex);
    }

    qemu_mutex_unlock(&wq->mutex);
}

void work_queue_destroy(struct work_queue *wq)
{
    wq->destroying = true;
    qemu_cond_signal(&wq->add_cond);

    qemu_thread_join(&wq->thread);

    qemu_cond_destroy(&wq->wait_cond);
    qemu_cond_destroy(&wq->add_cond);
    qemu_mutex_destroy(&wq->mutex);

    g_free(wq);
}
