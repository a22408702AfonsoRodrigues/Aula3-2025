#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include "queue.h"
#include <unistd.h>

/**
 * @brief Shortest Job First (SJF) scheduling algorithm (não-preemptivo).
 *
 * Se o CPU está ocupado, apenas atualiza o tempo e verifica se a tarefa terminou.
 * Se o CPU está livre, seleciona da ready queue a tarefa com menor tempo restante
 * (time_ms - ellapsed_time_ms). Em caso de empate, fica com a que aparece primeiro
 * na fila (ordem de chegada).
 *
 * @param current_time_ms Tempo atual em milissegundos.
 * @param rq              Ponteiro para a ready queue.
 * @param cpu_task        Duplo ponteiro para a tarefa atualmente no CPU.
 */
void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    // Se há tarefa no CPU, atualiza execução e verifica término (não-preemptivo)
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;  // acumula tempo de CPU

        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Tarefa terminou -> notificar a aplicação
            msg_t msg = (msg_t){
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            // Libertar PCB (seguindo o padrão do FIFO)
            free((*cpu_task));
            (*cpu_task) = NULL;
        }
    }

    // Se o CPU ficou livre, escolher próximo pelo menor tempo restante
    if (*cpu_task == NULL) {
        // Percorrer a fila ligada para encontrar o elemento com menor tempo restante
        queue_elem_t *it = rq->head;
        queue_elem_t *best_elem = NULL;

        uint32_t best_remaining = 0; // só válido quando best_elem != NULL

        while (it) {
            pcb_t *p = it->pcb;
            // tempo restante (protege contra underflow caso já tenha algum ellapsed)
            uint32_t remaining = (p->time_ms > p->ellapsed_time_ms)
                                 ? (p->time_ms - p->ellapsed_time_ms)
                                 : 0;
//tes
            if (best_elem == NULL || remaining < best_remaining) {
                best_elem = it;
                best_remaining = remaining;
            }
            it = it->next;
        }

        if (best_elem) {
            // Remover o elemento escolhido da fila
            queue_elem_t *removed = remove_queue_elem(rq, best_elem);
            if (removed) {
                *cpu_task = removed->pcb;   // entregar o PCB ao CPU
                free(removed);              // libertar o nó da fila (o PCB mantém-se)
            }
        }
    }
}
