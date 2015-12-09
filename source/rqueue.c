#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include "errno.h"

#include "rqueue.h"
#include "data.h"
#include "entry.h"

/* Inicialização de um array com n filas de tarefas.
 */
int init_task_queues(int n, struct q_head_t **nqueues){
	int i;
	int j;
	for (i = 0; i < n; i++){
		nqueues[i] = (struct q_head_t *) malloc(sizeof(struct q_head_t));
		nqueues[i]->queue_head = NULL;
		nqueues[i]->size = 0;
		if (pthread_mutex_init(&nqueues[i]->queue_lock, NULL) != 0){
			for(j = 0; j < i; j++){
				pthread_mutex_destroy(&nqueues[j]->queue_lock);
				pthread_cond_destroy(&nqueues[j]->queue_not_empty);
			}
			return -1;
		}
		if (pthread_cond_init(&nqueues[i]->queue_not_empty, NULL) != 0){
			for(j = 0; j < i; j++){
				pthread_mutex_destroy(&nqueues[j]->queue_lock);
				pthread_cond_destroy(&nqueues[j]->queue_not_empty);
			}
			pthread_mutex_destroy(&nqueues[i]->queue_lock);
			return -1;
		}
	}

	return 0;
}

/* Libertar memória relativa a n filas de tarefas.
 */
void destroy_task_queues(int n, struct q_head_t *nqueues){
	int i;
	struct qnode_t *aux;

	for (i = 0; i < n; i++){ /* Libertar nós em cada fila */
		while (nqueues[i].queue_head != NULL){
			aux = nqueues[i].queue_head;
			nqueues[i].queue_head = nqueues[i].queue_head->next;
			free(aux);
		}
		pthread_mutex_destroy(&nqueues[i].queue_lock);
		pthread_cond_destroy(&nqueues[i].queue_not_empty);
	}	
}

/* Inserir uma tarefa numa fila.
 */
int q_put_task(struct q_head_t *queue, struct quorum_op_t *task){
	struct qnode_t *qnode;
	struct qnode_t *aux;
	int res;
	if ( (res = pthread_mutex_lock(&queue->queue_lock)) != 0) /* Obter o MUTEX */
	{
		if(res == 22)
		{
			puts("MUTEX NAO INICIALIZADO");
		}else
		{
			puts("OUTRO ERRO");
		}
		return -1;
	}

	if ((qnode = (struct qnode_t *) malloc(sizeof(struct qnode_t))) == NULL) /* Criar um nó da fila */
		return -1;

	qnode->task = task; /* Inserir a tarefa */
	qnode->next = NULL;

	if (queue->queue_head == NULL) /* Colocar o nó no início da fila (cabeça da lista) */
		queue->queue_head = qnode;
	else{			       /* Colocar no fim da fila */
		aux = queue->queue_head;
		while (aux->next != NULL)
			aux = aux->next;
		aux->next = qnode;
	}
	queue->size = queue->size + 1;
	pthread_cond_signal(&queue->queue_not_empty);
	pthread_mutex_unlock(&queue->queue_lock); /* Libertar o MUTEX */

	return 0;
}

/* Retirar uma tarefa da fila
 */
struct quorum_op_t *q_get_task(struct q_head_t *queue){
	struct qnode_t *aux;
	struct quorum_op_t *rtask;
	int res;

	if ( (res = pthread_mutex_lock(&queue->queue_lock)) != 0) /* Obter o MUTEX */
	{
		if(res == 22)
		{
			puts("MUTEX NAO INICIALIZADO");
		}else
		{
			puts("OUTRO ERRO");
		}
		return NULL;
	}

	//se eh a fila de respostas
	if( queue->main == 1 )
	{
		struct timeval now;
		struct timespec timeToWait;

		//receber tempo actual
		gettimeofday(&now, NULL);

		//tempo de espera
		timeToWait.tv_sec = now.tv_sec;
		timeToWait.tv_nsec = now.tv_usec * 1000;
		timeToWait.tv_sec += 2;

		int wait;
		while(queue->queue_head == NULL && wait != ETIMEDOUT)
			wait = pthread_cond_timedwait(&queue->queue_not_empty, &queue->queue_lock, &timeToWait); /* Espera que exista uma tarefa */

		if( wait == ETIMEDOUT )
		{
			pthread_mutex_unlock(&queue->queue_lock);
			return NULL;
		}
	}
	//se eh uma fila de pedidos
	else
	{
		while(queue->queue_head == NULL)
			pthread_cond_wait(&queue->queue_not_empty, &queue->queue_lock);
	}

	aux = queue->queue_head;
	rtask = aux->task;
	queue->queue_head = aux->next; /* Retirar o nó da lista */ 
	free(aux); 

	queue->size = queue->size - 1;

	pthread_mutex_unlock(&queue->queue_lock); /* Libertar o MUTEX */

	return rtask;	
}

