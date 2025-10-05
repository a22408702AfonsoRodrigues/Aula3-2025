#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include "queue.h"
#include <unistd.h>


void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
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
            free((*cpu_task));
            (*cpu_task) = NULL;
        }
    }

    if (*cpu_task == NULL) {
        queue_elem_t *it = rq->head;
        queue_elem_t *best_elem = NULL;

        uint32_t best_remaining = 0;

        while (it) {
            pcb_t *p = it->pcb;
            uint32_t remaining = (p->time_ms > p->ellapsed_time_ms)
                                 ? (p->time_ms - p->ellapsed_time_ms)
                                 : 0;

            if (best_elem == NULL || remaining < best_remaining) {
                best_elem = it;
                best_remaining = remaining;
            }
            it = it->next;
        }

        if (best_elem) {
            queue_elem_t *removed = remove_queue_elem(rq, best_elem);
            if (removed) {
                *cpu_task = removed->pcb;
                free(removed);
            }
        }
    }
}
