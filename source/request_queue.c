#include <stdlib.h>
#include <pthread.h>

#include "request_queue.h"
#include "quorum_access.h"
#include "quorum_table.h"

/* Cria uma nova queue. Em caso de erro, retorna NULL.
 */
struct rqueue_t *rqueue_create() {

	struct rqueue_t *queue = (struct rqueue_t*) malloc(sizeof(struct rqueue_t));

	if (queue == NULL)
		return NULL;

	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;

	queue->queue_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(queue->queue_lock);

	queue->cond_queue_lock = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
	pthread_cond_init(&cond_queue_lock, NULL);

	return queue;
}


/* Elimina uma queue, libertando *toda* a memoria utilizada pela
 * queue.
 */
void queue_destroy(struct rqueue_t *queue) {
	struct rqueue_node_t *curr = queue->head;

	while(curr != NULL) {
		struct rqueue_node_t *next = curr->next;
		free(curr->request);
		free(curr);
		curr = next;
	}

	free(queue);
}


/* Adiciona um request na queue.
 * Retorna 0 (OK) ou -1 (erro)
 */
int rqueue_add(struct rqueue_t *queue, struct quorum_op_t *request) {

	struct rqueue_node_t *node =
		(struct rqueue_node_t*) malloc(sizeof(struct rqueue_node_t));

	node->request = request_dup(request);

	queue->tail->next = node;
	queue->tail       = node;
	queue->size++;

	return 0;
}


/* Retira da queue o elemento que estiver na tail.
 * Retorna 0 (OK) ou -1 (erro)
 */
struct quorum_op_t *rqueue_get(struct rqueue_t *queue) {
	return request_dup(queue->tail->request);
}


/* Retorna o tamanho (numero de elementos) da queue
 * Retorna -1 em caso de erro.  */
int rqueue_size(struct rqueue_t *queue) {
	return queue->size;
}


struct quorum_op_t *request_dup(struct quorum_op_t *request) {

	struct quorum_op_t *request_dup =
		(struct quorum_op_t*) malloc(sizeof(struct quorum_op_t));

	request_dup->id     = request->id;
	request_dup->sender = request->sender;
	request_dup->opcode = request->opcode;

	switch(request->opcode) {
		case OC_PUT:
			break;

		case OC_DEL:
			break;

		case OC_UPDATE:
			break;

		case OC_GET:
			break;

		case OC_SIZE:
			break;
	}

	return request_dup;
}