#include"threadpool.h"


threadpool_t *threadpool_create(int thread_count, int queue_size) {
    threadpool_t *pool = malloc(sizeof(threadpool_t));
    pool->threads = malloc(sizeof(pthread_t) * thread_count);
    pool->queue = malloc(sizeof(threadpool_task_t) * queue_size);
    pool->thread_count = thread_count;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = 0;
    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);
    for(int i = 0; i < thread_count; i++) {
        pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool);
    }
    return pool;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument) {
    pthread_mutex_lock(&(pool->lock));
    pool->queue[pool->tail].function = function;
    pool->queue[pool->tail].argument = argument;
    pool->tail = (pool->tail + 1) % pool->queue_size;
    pool->count += 1;
    //     // printf("A new task has been added to the thread pool.\n");
    // writePoolLog(" A new task has been added to the thread pool.\n",NULL);
    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));
    return 0;
}

void *threadpool_thread(void *threadpool) {
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;
    pthread_t threadId = pthread_self();
    while (1) {
        pthread_mutex_lock(&(pool->lock));
        while ((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }
        if (pool->shutdown) {
            break;
        }
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;
        //  printf("A task is being executed by a thread.\n");
        writePoolLog(" A task is being executed by a thread.\n",threadId);
        pthread_mutex_unlock(&(pool->lock));
        (*(task.function))(task.argument);
        // printf("A task has finished execution.\n");
        writePoolLog(" A task has finished execution.\n",threadId);
    }
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return(NULL);
}