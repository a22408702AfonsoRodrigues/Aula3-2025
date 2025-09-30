#ifndef RR_H
#define RR_H

#include "queue.h"

// Round-Robin com time-slice (quantum) de 500 ms
void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task);

#endif // RR_H