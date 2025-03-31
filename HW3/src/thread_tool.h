#ifndef THREAD_TOOL_H
#define THREAD_TOOL_H

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

// The maximum number of threads.
#define THREAD_MAX 100


void sighandler(int signum);
void scheduler();

// The thread control block structure.
struct tcb {
    int id;
    int *args;
    // Reveals what resource the thread is waiting for. The values are:
    //  - 0: no resource.
    //  - 1: read lock.
    //  - 2: write lock.
    int waiting_for;
    int sleeping_time;
    jmp_buf env;  // Where the scheduler should jump to.
    int n, i, f_cur, f_prev; // TODO: Add some variables you wish to keep between switches.
    //存其他東西
    int q_s, q_p, d_p, d_s, s, b, p_p, p_s; //for enroll
};

// The only one thread in the RUNNING state.
extern struct tcb *current_thread;
extern struct tcb *idle_thread;

struct tcb_queue {
    struct tcb *arr[THREAD_MAX];  // The circular array.
    int head;                     // The index of the head of the queue
    int size;
};

extern struct tcb_queue ready_queue, waiting_queue;


// The rwlock structure.
//
// When a thread acquires a type of lock, it should increment the corresponding count.
struct rwlock {
    int read_count;
    int write_count;
};

extern struct rwlock rwlock;

// The remaining spots in classes.
extern int q_p, q_s;

// The maximum running time for each thread.
extern int time_slice;

// The long jump buffer for the scheduler.
extern jmp_buf sched_buf;

// TODO::
// You should setup your own sleeping set as well as finish the marcos below

extern struct tcb *sleeping_set[THREAD_MAX];
//each entry of sleeping_set saves the thread with that id

#define thread_create(func, t_id, t_args)                                              \
    ({                                                                                 \
        func(t_id, t_args);                                                            \
    })

#define thread_setup(t_id, t_args)                                                     \
    ({                                                                                 \
        struct tcb *new_thread = (struct tcb *)malloc(sizeof(struct tcb));             \
        new_thread->id = t_id;                                                          \
        new_thread->args = t_args;                                                     \
        new_thread->waiting_for = 0;                                                   \
        new_thread->sleeping_time = 0;                                                 \
        printf("thread %d: set up routine %s\n", t_id, __func__);                      \
        if (t_id == 0){                                                                \
            idle_thread = new_thread;                                                  \
        } else {                                                                       \
            ready_queue.arr[ready_queue.head + ready_queue.size] = new_thread;         \
            ready_queue.size++;                                                        \
        }                                                                              \
        current_thread = new_thread;                                                   \
        if (setjmp(new_thread->env) == 0) {                                            \
            return;                                                                    \
        }                                                                              \
    })

#define thread_yield()                                  \
    ({                                                  \
        if (setjmp(current_thread->env) == 0) {        \
            sigset_t set;                              \
            sigemptyset(&set);                         \
            sigaddset(&set, SIGTSTP);                  \
            sigprocmask(SIG_UNBLOCK, &set, NULL);      \
            sigprocmask(SIG_BLOCK, &set, NULL);        \
            sigemptyset(&set);                         \
            sigaddset(&set, SIGALRM);                  \
            sigprocmask(SIG_UNBLOCK, &set, NULL);      \
            sigprocmask(SIG_BLOCK, &set, NULL);        \
        }                                              \
    })

#define read_lock()                                                              \
    ({                                                                           \
        while (1) {                                                              \
            if (rwlock.write_count == 0) {                                       \
                rwlock.read_count++;                                             \
                current_thread->waiting_for = 0;                                \
                break;                                                           \
            }                                                                    \
            current_thread->waiting_for = 1;                                     \
            if (setjmp(current_thread->env) == 0) {                              \
                longjmp(sched_buf, 2);                                           \
            }                                                                    \
        }                                                                        \
    })

#define write_lock()                                                              \
    ({                                                                            \
        while (1) {                                                               \
            if (rwlock.write_count == 0 && rwlock.read_count == 0) {              \
                rwlock.write_count = 1;                                           \
                current_thread->waiting_for = 0;                                  \
                break;                                                            \
            }                                                                     \
            current_thread->waiting_for = 2;                                      \
            if (setjmp(current_thread->env) == 0) {                               \
                longjmp(sched_buf, 2);                                            \
            }                                                                     \
        }                                                                         \
    })

#define read_unlock()                                                                 \
    ({                                                                                \
        rwlock.read_count--;                                                         \
    })

#define write_unlock()                                                                \
    ({                                                                                \
        rwlock.write_count = 0;                                                      \
    })

#define thread_sleep(sec)                                            \
    ({                                                               \
        current_thread->sleeping_time = sec;                         \
        sleeping_set[current_thread->id] = current_thread;           \
        if (setjmp(current_thread->env) == 0) {                      \
            longjmp(sched_buf, 3);                                   \
        }                                                            \
    })

#define thread_awake(t_id)                                                        \
    ({                                                                            \
        if (sleeping_set[t_id] != NULL) {                                         \
            current_thread->sleeping_time = 0;                               \
            ready_queue.arr[ready_queue.head + ready_queue.size] = sleeping_set[t_id]; \
            ready_queue.size++;                                                  \
            sleeping_set[t_id] = NULL;                                           \
        }                                                                        \
    })

#define thread_exit()                                    \
    ({                                                   \
        printf("thread %d: exit\n", current_thread->id); \
        longjmp(sched_buf, 4);                           \
    })

#endif  // THREAD_TOOL_H
