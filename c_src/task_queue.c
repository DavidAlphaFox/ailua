#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#include "task_queue.h"

struct _task_item
{
    struct _task_item* next;
    void* data;
};

typedef struct _task_item task_item_t;

struct _task_queue
{
    ErlNifMutex* lock;
    ErlNifCond* cond;
    task_item_t* head;
    task_item_t* tail;
    void* message;
    int length;
    bool waiting;
};

task_queue_t*
task_queue_alloc()
{
    task_queue_t* ret;

    ret = (task_queue_t *) enif_alloc(sizeof(struct _task_queue));
    if(ret == NULL) goto error;
    ret->lock = NULL;
    ret->cond = NULL;
    ret->head = NULL;
    ret->tail = NULL;
    ret->message = NULL;
    ret->length = 0;
    ret->waiting = false;

    ret->lock = enif_mutex_create("task_queue_lock");
    if(NULL == ret->lock) goto error;

    ret->cond = enif_cond_create("task_queue_cond");
    if(NULL == ret->cond) goto error;

    return ret;
error:
    if (NULL == ret) return NULL;
    if(NULL != ret->lock) enif_mutex_destroy(ret->lock);
    if(NULL != ret->cond) enif_cond_destroy(ret->cond);
    enif_free(ret);
    return NULL;
}

void
task_queue_free(task_queue_t* queue)
{
    ErlNifMutex *lock;
    ErlNifCond *cond;
    int length;

    enif_mutex_lock(queue->lock);
    lock = queue->lock;
    cond = queue->cond;
    length = queue->length;

    queue->lock = NULL;
    queue->cond = NULL;
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
    queue-> waiting = false;
    enif_mutex_unlock(lock);

    assert(length == 0 && "Attempting to destroy a non-empty queue.");
    enif_cond_destroy(cond);
    enif_mutex_destroy(lock);
    enif_free(queue);
}


int
task_queue_push(task_queue_t* queue, void* item)
{
    task_item_t * entry = (task_item_t *) enif_alloc(sizeof(struct _task_item));
    if(NULL == entry) return 0;

    entry->data = item;
    entry->next = NULL;

    enif_mutex_lock(queue->lock);

    assert(queue->length >= 0 && "Invalid queue size at push");

    if(NULL != queue->tail)
        queue->tail->next = entry;

    queue->tail = entry;

    if(NULL == queue->head )
        queue->head = queue->tail;

    queue->length += 1;

    if(queue->waiting){
        enif_cond_signal(queue->cond);
    }
    enif_mutex_unlock(queue->lock);

    return 1;
}

void*
task_queue_pop(task_queue_t* queue)
{
    task_item_t* entry;
    void* item;

    enif_mutex_lock(queue->lock);

    /* Wait for an item to become available.
     */
    while(NULL == queue->head){
        queue->waiting = true;
        enif_cond_wait(queue->cond, queue->lock);
    }
    queue->waiting = false;
        

    assert(queue->length >= 0 && "Invalid queue size at pop.");

    /* Woke up because queue->head != NULL
     * Remove the entry and return the payload.
     */
    entry = queue->head;
    queue->head = entry->next;
    entry->next = NULL;

    if(NULL == queue->head) {
        assert(queue->tail == entry && "Invalid queue state: Bad tail.");
        queue->tail = NULL;
    }

    queue->length -= 1;

    enif_mutex_unlock(queue->lock);

    item = entry->data;
    enif_free(entry);

    return item;
}
