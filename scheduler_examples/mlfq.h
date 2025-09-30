#ifndef MLFQ_H
#define MLFQ_H

#include "queue.h"

// Multi-Level Feedback Queue com time-slice de 500 ms
void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task);

#endif // MLFQ_H