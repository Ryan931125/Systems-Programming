#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "routine.h"
#include "thread_tool.h"

// TODO::
// Prints out the signal you received.
// This function should not return. Instead, jumps to the scheduler.
void sighandler(int signum) {
    if (signum == SIGTSTP)
        printf("caught SIGTSTP\n");
    else if (signum == SIGALRM)
        printf("caught SIGALRM\n");
    longjmp(sched_buf, 1);
}

// TODO::
// Perfectly setting up your scheduler.
void scheduler() {
    //first time setup
    if (idle_thread == NULL) {
        thread_create(idle, 0, NULL);
    }
    int jump_val;
    if ((jump_val = setjmp(sched_buf)) == -1){
        perror("setjmp sched_buf");
        exit(1);
    }

    // Reset alarm and clear pending signals
    //set new alarm
    alarm(0);
    alarm(time_slice);

    // Clear pending signals by temporarily setting them to SIG_IGN
    struct sigaction sa_ignore, sa_old;
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    
    sigaction(SIGTSTP, &sa_ignore, &sa_old);
    sigaction(SIGTSTP, &sa_old, NULL);
    sigaction(SIGALRM, &sa_ignore, &sa_old);
    sigaction(SIGALRM, &sa_old, NULL);


    // __sighandler_t old_handler = signal(SIGTSTP, SIG_IGN);
    // signal(SIGTSTP, old_handler);
    // old_handler = signal(SIGALRM, SIG_IGN);
    // signal(SIGALRM, old_handler);

    // Manage sleeping threads
    if (jump_val == 1) {
        for (int i = 0; i < THREAD_MAX; i++) {
            if (sleeping_set[i]) {
                sleeping_set[i]->sleeping_time -= time_slice;
                if (sleeping_set[i]->sleeping_time <= 0) {
                    ready_queue.arr[ready_queue.head + ready_queue.size] = sleeping_set[i];
                    ready_queue.size++;
                    sleeping_set[i] = NULL;
                }
            }
        }
    }

    // Handle waiting threads
    bool moved;
    do {
        moved = false;
        if (waiting_queue.size > 0) {
            struct tcb *waiting_thread = waiting_queue.arr[waiting_queue.head];
            if ((waiting_thread->waiting_for == 1 && rwlock.write_count == 0) ||
                (waiting_thread->waiting_for == 2 && rwlock.write_count == 0 && rwlock.read_count == 0)) {
                ready_queue.arr[ready_queue.head + ready_queue.size] = waiting_thread;
                ready_queue.size++;
                waiting_queue.head = (waiting_queue.head + 1) % THREAD_MAX;
                waiting_queue.size--;
                moved = true;
            }
        }
    } while (moved);

    // Handle previous running thread
    // int jump_val = setjmp(sched_buf);
    if (current_thread != NULL && current_thread != idle_thread) {
        if (jump_val == 1) {  // From yield
            ready_queue.arr[ready_queue.head + ready_queue.size] = current_thread;
            ready_queue.size++;
        } else if (jump_val == 2) {  // From lock
            waiting_queue.arr[waiting_queue.head + waiting_queue.size] = current_thread;
            waiting_queue.size++;
        } else if (jump_val == 3) {  // From sleep
            //do nothing
        } else if (jump_val == 4) {  // From exit
            free(current_thread->args);
            free(current_thread);
        }
    }

    // Select next thread
    if (ready_queue.size > 0) {
        current_thread = ready_queue.arr[ready_queue.head];
        ready_queue.head = (ready_queue.head + 1) % THREAD_MAX;
        ready_queue.size--;
    } else {
        bool has_sleeping = false;
        for (int i = 0; i < THREAD_MAX; i++) {
            if (sleeping_set[i] != NULL) {
                has_sleeping = true;
                break;
            }
        }
        if (!has_sleeping) {
            if (idle_thread != NULL) {
                free(idle_thread);
            }
            return;
        }
        current_thread = idle_thread;
    }

    longjmp(current_thread->env, 1);
}
