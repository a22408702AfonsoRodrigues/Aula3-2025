#include "rr.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include "queue.h"
#include <unistd.h>

#define RR_QUANTUM_MS 500

/**
 * @brief Round-Robin scheduler (preemptivo) com quantum de 500 ms.
 *
 * - Se houver tarefa no CPU: acumula tempo decorrido; se terminar, envia DONE e liberta PCB.
 * - Se não terminou e o quantum expirou:
 *      - Se houver alguém na ready queue, faz preempção: envia a tarefa atual para o fim da fila e escolhe a próxima.
 *      - Se a fila estiver vazia, deixa a tarefa continuar e renova o slice.
 * - Se o CPU estiver livre, vai buscar a próxima da fila.
 */
void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    // 1) Atualizar a tarefa atual (se existir)
    if (*cpu_task) {
        // Avança tempo de CPU
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // Terminou?
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
            *cpu_task = NULL; // CPU fica livre
        } else {
            // Verifica quantum
            uint32_t slice_elapsed = current_time_ms - (*cpu_task)->slice_start_ms;
            if (slice_elapsed >= RR_QUANTUM_MS) {
                // Só preempta se houver alguém à espera
                if (rq->head != NULL) {
                    // Reenfileira a tarefa atual no fim da fila
                    pcb_t *to_requeue = *cpu_task;
                    *cpu_task = NULL;
                    enqueue_pcb(rq, to_requeue);
                } else {
                    // Ninguém à espera: renova o slice e continua
                    (*cpu_task)->slice_start_ms = current_time_ms;
                }
            }
        }
    }

    // 2) Se o CPU está livre, escolhe próxima tarefa
    if (*cpu_task == NULL) {
        pcb_t *next = dequeue_pcb(rq);
        if (next) {
            *cpu_task = next;
            (*cpu_task)->status = TASK_RUNNING;
            (*cpu_task)->slice_start_ms = current_time_ms;  // inicia novo time-slice
        }
    }
}