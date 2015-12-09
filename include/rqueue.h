#ifndef _RQUEUE_H
#define _RQUEUE_H

#include "quorum_access.h"
#include <pthread.h>

struct qnode_t {     /* Nó de uma fila de tarefas */
	struct quorum_op_t *task;
	struct qnode_t *next;
};

struct q_head_t {
	int size;
	int main;
	struct qnode_t *queue_head;
	pthread_mutex_t queue_lock;
	pthread_cond_t queue_not_empty;
};

/* Inicialização de um array com n filas de tarefas.
 */
int init_task_queues(int n, struct q_head_t **nqueues);

/* Libertar memória relativa a n filas de tarefas.
 */
void destroy_task_queues(int n, struct q_head_t *nqueues);

/* Inserir uma tarefa numa fila.
 */
int q_put_task(struct q_head_t *queue, struct quorum_op_t *task);

/* Retirar uma tarefa da fila
 */
struct quorum_op_t *q_get_task(struct q_head_t *queue);

#endif
