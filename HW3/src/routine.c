#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_tool.h"

void idle(int id, int *args) {
    thread_setup(id, args);

    while (1) {
        printf("thread %d: idle\n", current_thread->id);
        sleep(1);
        thread_yield();
    }
}

void fibonacci(int id, int *args) {
    thread_setup(id, args);

    current_thread->n = current_thread->args[0];
    for (current_thread->i = 1;; current_thread->i++) {
        if (current_thread->i <= 2) {
            current_thread->f_cur = 1;
            current_thread->f_prev = 1;
        } else {
            int f_next = current_thread->f_cur + current_thread->f_prev;
            current_thread->f_prev = current_thread->f_cur;
            current_thread->f_cur = f_next;
        }
        printf("thread %d: F_%d = %d\n", current_thread->id, current_thread->i,
               current_thread->f_cur);

        sleep(1);

        if (current_thread->i == current_thread->n) {
            thread_exit();
        } else {
            thread_yield();
        }
    }
}

void pm(int id, int *args) {
    thread_setup(id, args);

    current_thread->n = current_thread->args[0];
    for (current_thread->i = 1;; current_thread->i++) {
        if (current_thread->i == 1) {
            current_thread->f_cur = 1;
            // current_thread->f_prev = 1;
        } else {
            int sign = current_thread->i % 2 == 0 ? -1 : 1;
            current_thread->f_cur = current_thread->f_cur + sign * current_thread->i;
        }
        printf("thread %d: pm(%d) = %d\n", current_thread->id, current_thread->i,
               current_thread->f_cur);

        sleep(1);

        if (current_thread->i == current_thread->n) {
            thread_exit();
        } else {
            thread_yield();
        }
    }
}

void enroll(int id, int *args) {
    thread_setup(id, args);

    current_thread->d_p = current_thread->args[0];
    current_thread->d_s = current_thread->args[1];
    current_thread->s = current_thread->args[2];
    current_thread->b = current_thread->args[3];

    // Step 1: Sleep
    printf("thread %d: sleep %d\n", current_thread->id, current_thread->s);
    thread_sleep(current_thread->s);
    
    // Step 2: Wake up friend and read lock
    thread_awake(current_thread->b);
    read_lock();
    current_thread->q_p = q_p;
    current_thread->q_s = q_s;
    printf("thread %d: acquire read lock\n", current_thread->id);
    sleep(1);
    thread_yield();
    
    // Step 3: Release read lock and compute priorities
    read_unlock();
    current_thread->p_p = current_thread->d_p * q_p;
    current_thread->p_s = current_thread->d_s * q_s;
    printf("thread %d: release read lock, p_p = %d, p_s = %d\n", 
           current_thread->id, current_thread->p_p, current_thread->p_s);
    sleep(1);
    thread_yield();
    
    // Step 4: Acquire write lock and enroll
    write_lock();
    if (q_p == 0){//pj class is full
        q_s--;
        printf("thread %d: acquire write lock, enroll in sw_class\n", current_thread->id);
    }
    else if (q_s == 0){//sw class is full
        q_p--;
        printf("thread %d: acquire write lock, enroll in pj_class\n", current_thread->id);
    }
    else if (current_thread->p_p == current_thread->p_s) {//priority is equal
        if (current_thread->d_p > current_thread->d_s) {
            q_p--;
            printf("thread %d: acquire write lock, enroll in pj_class\n", current_thread->id);
        } else {
            q_s--;
            printf("thread %d: acquire write lock, enroll in sw_class\n", current_thread->id);
        }
    }
    else if (current_thread->p_p > current_thread->p_s) {//priority of pj is higher
        q_p--;
        printf("thread %d: acquire write lock, enroll in pj_class\n", current_thread->id);
    } 
    else{//priority of sw is higher
        q_s--;
        printf("thread %d: acquire write lock, enroll in sw_class\n", current_thread->id);
    }
    
    sleep(1);
    thread_yield();
    
    // Step 5: Release write lock and exit
    write_unlock();
    printf("thread %d: release write lock\n", current_thread->id);
    sleep(1);
    thread_exit();
}

