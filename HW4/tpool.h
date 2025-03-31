#pragma once

#include <pthread.h>
#include <stdbool.h>

typedef int** Matrix;
typedef int* Vector;

// Structure for a work item (dot product calculation)
struct work_item {
    Matrix a;
    Matrix b_trans;
    Matrix c;
    int n;          // Matrix dimension
    int start_idx;  // Starting index for this work
    int end_idx;    // Ending index for this work
    struct work_item* next;
};

// Structure for a request
struct request {
    Matrix a;
    Matrix b;
    Matrix c;
    int num_works;
    struct request* next;
};

// Queue structures
struct queue {
    void* head;
    void* tail;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
};

struct tpool {
    // Thread management
    pthread_t frontend_thread;
    pthread_t* backend_threads;
    int num_threads;
    int n;
    
    // Queues
    struct queue frontend_queue;  // For requests
    struct queue worker_queue;    // For work items
    
    // Synchronization
    int works_left;           // Number of tasks not yet completed
    bool shutdown;
    pthread_mutex_t work_lock;
    pthread_cond_t no_works_left;
};

struct tpool* tpool_init(int num_threads, int n);
void tpool_request(struct tpool*, Matrix a, Matrix b, Matrix c, int num_works);
void tpool_synchronize(struct tpool*);
void tpool_destroy(struct tpool*);
int calculation(int n, Vector, Vector);