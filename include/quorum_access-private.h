#ifndef _QUORUM_ACCESS_PRIVATE_H
#define _QUORUM_ACCESS_PRIVATE_H

#include "rqueue.h"
struct thread_data{
	int id;
	int state;
	pthread_t thread;
	struct q_head_t *in_queue;
	struct q_head_t *out_queue;
	struct rtable_t *rtable;
};


void *quorum_send(void *arg);

void *quorum_get_size(void *arg);

/*
	Funcao que lanca num_threads com inicio em *f
*/
void thread_launcher(void (*f)(int), int num_threads, struct thread_data **threads_data);

/*
	Funcao que remove pedido de uma queue
*/
struct quorum_op_t *get_from_queue(struct rqueue_t *queue);

/*
	Funcao que envia put para servidor de thread
*/
void quorum_put( struct thread_data *thread );

void *server_access( void * arg );


// operacoes comuns
struct quorum_op_t **send_get_timestamp_to_k(struct quorum_op_t *request);
struct quorum_op_t **send_timestamp_until_k( struct quorum_op_t *request );

//
int get_most_recent_pos(struct quorum_op_t **arr);

int replace_server(int pos);

int is_active_server(int id);

int get_inactive_server(struct thread_data **servers, int n);

int find_most_recent_id(struct quorum_op_t **replies);

int find_pos_by_id(struct thread_data **ths, int id, int n);

int get_available_server();

struct quorum_op_t *create_quorum_op_ts(int id, char *key);

int replace_inactive_server(int active_pos);

int refresh_active_servers(struct rtable_t **rtable);

#endif
