#include "rr.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include "queue.h"
#include <unistd.h>

#define RR_QUANTUM_MS 500



void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
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
            if (slice_elapsed >= RR_QUANTUM_MS) {
                if (rq->head != NULL) {
                    pcb_t *to_requeue = *cpu_task;
                    *cpu_task = NULL;
                    enqueue_pcb(rq, to_requeue);
                } else {
                    (*cpu_task)->slice_start_ms = current_time_ms;
                }
            }
        }
    }

    if (*cpu_task == NULL) {
        pcb_t *next = dequeue_pcb(rq);
        if (next) {
            *cpu_task = next;
            (*cpu_task)->status = TASK_RUNNING;
            (*cpu_task)->slice_start_ms = current_time_ms;
        }
    }
}