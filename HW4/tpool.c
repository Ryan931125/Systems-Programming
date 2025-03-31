#include "tpool.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper function to transpose matrix
static Matrix transpose_matrix(Matrix m, int n) {
    Matrix trans = malloc(n * sizeof(Vector));
    trans[0] = malloc(n * n * sizeof(int));
    for (int i = 1; i < n; i++) {
        trans[i] = trans[0] + i * n;
    }
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            trans[i][j] = m[j][i];
        }
    }
    return trans;
}

static void queue_init(struct queue* q) {
    if (!q) return;

    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

static void frontend_queue_push(struct queue* q, struct request* req) {
    if (!q || !req) return;

    pthread_mutex_lock(&q->lock);
    req->next = NULL;
    
    if (q->tail == NULL) {
        q->head = req;
        q->tail = req;
    } else {
        ((struct request*)q->tail)->next = req;
        q->tail = req;
    }
    
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

static struct request* frontend_queue_pop(struct queue* q, struct tpool* pool) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->lock);
    while (q->head == NULL && !pool->shutdown){
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    
    if (q->head == NULL) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    
    struct request* req = q->head;
    q->head = req->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    
    pthread_mutex_unlock(&q->lock);
    return req;
}

static void worker_queue_push(struct queue* q, struct work_item* work) {
    if (!q || !work) return;

    pthread_mutex_lock(&q->lock);
    work->next = NULL;
    
    if (q->tail == NULL) {
        q->head = work;
        q->tail = work;
    } else {
        ((struct work_item*)q->tail)->next = work;
        q->tail = work;
    }
    
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

static struct work_item* worker_queue_pop(struct queue* q, struct tpool* pool) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->lock);
    while(q->head == NULL && !pool->shutdown){
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    
    if (q->head == NULL) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    
    struct work_item* work = q->head;
    q->head = work->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    
    pthread_mutex_unlock(&q->lock);
    return work;
}

static void queue_destroy(struct queue* q) {
    if (!q) return;
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
}

static void* frontend_function(void* arg) {
    if (!arg) return NULL;

    struct tpool* pool = (struct tpool*)arg;
    if (!pool) return NULL;
    
    while (!pool->shutdown) {
        struct request* req = frontend_queue_pop(&pool->frontend_queue, pool);
        if (!req) break; //shutdown
        
        Matrix b_trans = transpose_matrix(req->b, pool->n);
        int total_cells = pool->n * pool->n;
        int cells_per_work = (total_cells + req->num_works - 1) / req->num_works;
        
        for (int i = 0; i < req->num_works; i++) {
            struct work_item* work = malloc(sizeof(struct work_item));
            work->a = req->a;
            work->b_trans = b_trans;
            work->c = req->c;
            work->n = pool->n;
            work->start_idx = i * cells_per_work;
            work->end_idx = (i + 1) * cells_per_work;
            if (work->end_idx > total_cells) {
                work->end_idx = total_cells;
            }
            work->next = NULL;
            worker_queue_push(&pool->worker_queue, work);
        }
        free(req);
    }
    return NULL;
}

static void* backend_function(void* arg) {
    if (!arg) return NULL;
    struct tpool* pool = (struct tpool*)arg;
    
    while (!pool->shutdown) {
        struct work_item* work = worker_queue_pop(&pool->worker_queue, pool);
        if (!work) break; //shutdown
        
        for (int idx = work->start_idx; idx < work->end_idx; idx++) {
            int i = idx / work->n;
            int j = idx % work->n;
            work->c[i][j] = calculation(work->n, work->a[i], work->b_trans[j]);
        }
        
        pthread_mutex_lock(&pool->work_lock);
        pool->works_left--;
        if (pool->works_left == 0) {
            pthread_cond_signal(&pool->no_works_left);
        }
        pthread_mutex_unlock(&pool->work_lock);

        free(work);
    }
    
    return NULL;
}

struct tpool* tpool_init(int num_threads, int n) {
    struct tpool* pool = malloc(sizeof(struct tpool));
    
    pool->num_threads = num_threads;
    pool->n = n;
    pool->shutdown = false;
    pool->works_left = 0;
    
    queue_init(&pool->frontend_queue);
    queue_init(&pool->worker_queue);
    pthread_mutex_init(&pool->work_lock, NULL);
    pthread_cond_init(&pool->no_works_left, NULL);
    
    pool->backend_threads = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->backend_threads[i], NULL, backend_function, pool);
    }
    pthread_create(&pool->frontend_thread, NULL, frontend_function, pool);
    
    return pool;
}

void tpool_request(struct tpool* pool, Matrix a, Matrix b, Matrix c, int num_works) {
    if (!pool || !a || !b || !c || num_works <= 0) return;

    struct request* req = malloc(sizeof(struct request));
    req->a = a;
    req->b = b;
    req->c = c;
    req->num_works = num_works;
    req->next = NULL;
    
    frontend_queue_push(&pool->frontend_queue, req);

    pthread_mutex_lock(&pool->work_lock);
    pool->works_left += num_works;
    pthread_mutex_unlock(&pool->work_lock);
}

void tpool_synchronize(struct tpool* pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->work_lock);
    while (pool->works_left > 0){
        pthread_cond_wait(&pool->no_works_left, &pool->work_lock);
    }
    pthread_mutex_unlock(&pool->work_lock);
}

void tpool_destroy(struct tpool* pool) {
    pool->shutdown = true;

    pthread_mutex_lock(&pool->frontend_queue.lock);
    pthread_cond_broadcast(&pool->frontend_queue.not_empty);
    pthread_mutex_unlock(&pool->frontend_queue.lock);
    
    pthread_mutex_lock(&pool->worker_queue.lock);
    pthread_cond_broadcast(&pool->worker_queue.not_empty);
    pthread_mutex_unlock(&pool->worker_queue.lock);

    
    // Join all threads
    pthread_join(pool->frontend_thread, NULL);
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->backend_threads[i], NULL);
    }
    
    // Clean up resources
    queue_destroy(&pool->frontend_queue);
    queue_destroy(&pool->worker_queue);
    pthread_mutex_destroy(&pool->work_lock);
    pthread_cond_destroy(&pool->no_works_left);
    
    free(pool->backend_threads);
    free(pool);
}