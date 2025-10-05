#include "mlfq.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include "queue.h"
#include <unistd.h>

#define MLFQ_QUANTUM_MS 500

static queue_t q0 = {0}, q1 = {0}, q2 = {0};

static int running_level = 0;

static inline int queues_nonempty(void) {
    return (q0.head != NULL) || (q1.head != NULL) || (q2.head != NULL);
}

static inline void drain_new_arrivals_into_q0(queue_t *rq) {
    pcb_t *p;
    while ((p = dequeue_pcb(rq)) != NULL) {
        enqueue_pcb(&q0, p);
    }
}

static inline pcb_t* get_next_from_queues(int *level_out) {
    if (q0.head) { *level_out = 0; return dequeue_pcb(&q0); }
    if (q1.head) { *level_out = 1; return dequeue_pcb(&q1); }
    if (q2.head) { *level_out = 2; return dequeue_pcb(&q2); }
    return NULL;
}

static inline void requeue_demoted(pcb_t *p, int from_level) {
    if (from_level == 0) {
        enqueue_pcb(&q1, p);
    } else if (from_level == 1) {
        enqueue_pcb(&q2, p);
    } else {
        enqueue_pcb(&q2, p);
    }
}


void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    drain_new_arrivals_into_q0(rq);

    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = (msg_t){
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free(*cpu_task);
            *cpu_task = NULL;
        } else {
            uint32_t slice_elapsed = current_time_ms - (*cpu_task)->slice_start_ms;
            if (slice_elapsed >= MLFQ_QUANTUM_MS) {
                if (queues_nonempty()) {
                    pcb_t *to_demote = *cpu_task;
                    *cpu_task = NULL;
                    requeue_demoted(to_demote, running_level);
                } else {
                    (*cpu_task)->slice_start_ms = current_time_ms;
                }
            }
        }
    }

    if (*cpu_task == NULL) {
        pcb_t *next = get_next_from_queues(&running_level);
        if (next) {
            *cpu_task = next;
            (*cpu_task)->status = TASK_RUNNING;
            (*cpu_task)->slice_start_ms = current_time_ms;
        }
    }
}
