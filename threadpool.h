#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "logevent.h"

typedef struct
{
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

typedef struct
{
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
} threadpool_t;
threadpool_t *threadpool_create(int thread_count, int queue_size);
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument);
void *threadpool_thread(void *threadpool);

#endif
