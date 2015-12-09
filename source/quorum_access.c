#include <pthread.h>
#include "network_client-private.h"
#include "message-private.h"
#include "quorum_access.h"
#include "quorum_table-private.h"
#include "quorum_access-private.h"
#include "client_stub-private.h"
#include "rqueue.h"
#include "inet.h"

//fila de respostas
struct q_head_t *replies_queue;

//array de thread_data geral
struct thread_data **threads_data;

//array com thread_data de 
struct thread_data **active_threads;

//numero de threads
int num_threads;

//numero de threads activas
int num_active_threads;

struct rtable_t **rtable;


struct q_head_t **queues;
/* Esta função deve criar as threads e as filas de comunicação para que o
 * cliente invoque operações a um conjunto de tabelas em servidores
 * remotos. Recebe como parâmetro um array rtable de tamanho n.
 * Retorna 0 (OK) ou -1 (erro).
 */
int init_quorum_access(struct rtable_t *rtable, int n) {

	//numero de threads possiveis
	num_threads = n;

	//calcula o numero de threads minimas que tem que ter
	num_active_threads = floor( n / 2 ) + 1;
	queues = (struct q_head_t **) malloc(sizeof(struct q_head_t *) * num_threads);

	int i;
	if (init_task_queues(num_threads + 1, queues) != 0){ /* Cria array de filas */
		perror("Impossível criar filas.");
		return -1;
	}

	//queue de respostas
	replies_queue = queues[num_threads];
	replies_queue->main = 1;

	//inicializa vector de threads totais
	threads_data = (struct thread_data **) malloc( sizeof( struct thread_data * ) * num_threads );
	for( i = 0; i < num_threads; i++ )
	{
		threads_data[i] = (struct thread_data *) malloc(sizeof(struct thread_data));
		threads_data[i]->id = i;
		threads_data[i]->in_queue = queues[i];
		threads_data[i]->out_queue = replies_queue;
		threads_data[i]->rtable = &rtable[i];
	}

	//printf("NUM ACTIVAS: %d\n", num_active_threads);

	//inicializa array de threads activas
	active_threads = (struct thread_data **) malloc( sizeof( struct thread_data * ) * num_active_threads );
	for(i = 0; i < num_active_threads; i++)
		active_threads[i] = NULL;

	//obtem servers mais activos
	int num_servers_r = refresh_active_servers(rtable); 

	return 0;
}

void *server_access(void *p){

	int disconnected = 0;
	struct thread_data *thread = (struct thread_data *) p;

	while(!disconnected)
	{
		//espera por obter pedido
		//printf("THREAD %d aguarda por pedido", thread->id);
		struct quorum_op_t *task = q_get_task(thread->in_queue);
		if( task == NULL){
			return NULL;
		}

		struct quorum_op_t *rep;
		struct quorum_op_t *reply;
		int res;
		//se eh para saber os servers que estão activos
		if( task->sender == -1 )
		{
			int res = rtable_size(thread->rtable);
			thread->state = 1;
			//printf("THREAD ID: %d RECEBEU SIZE\n", thread->id);
			rep = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t)); 
			rep->opcode = thread->id;
			//char *eye = strdup("teste");
			q_put_task(thread->out_queue, rep );

			//printf("THREAD: %d libertou locker de out", thread->id);
		}

		//se eh para fazer um request ao servidor

		//UPDATE
		if( task->opcode == OC_UPDATE )
		{
			res = rtable_update(thread->rtable, task->content.entry->key, task->content.entry->value);
			struct quorum_op_t *r = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
			r->opcode = OC_UPDATE;
			r->id = task->id;
			r->sender = thread->id;
			r->content.result = res;
			q_put_task(thread->out_queue, r);
		}
		//PUT
		else if( task->opcode == OC_PUT )
		{
			res = rtable_put(thread->rtable, task->content.entry->key, task->content.entry->value);
			thread->state = 1;
			reply = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
			reply->content.result = res;
			reply->id = task->id;
			reply->opcode = OC_PUT;
			reply->sender = thread->id;
			q_put_task( thread->out_queue, reply );
		}
		//GET
		else if( task->opcode == OC_GET )
		{
			struct data_t *data = rtable_get( thread->rtable, task->content.key );
			thread->state = 1;

			reply = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
			reply->content.data = data;
			reply->id = task->id;
			reply->opcode = OC_GET;
			reply->sender = thread->id;

			q_put_task(thread->out_queue, reply);
		}
		//TIMESTAMP
		else if( task->opcode == OC_RT_GETTS )
		{
			long long ts = rtable_get_ts(thread->rtable, task->content.key);
			reply = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
			reply->content.timestamp = ts;
			reply->sender = thread->id;
			reply->opcode = OC_RT_GETTS;
			reply->id = task->id;
			q_put_task(thread->out_queue, reply);
		}
		//NUM_OPS
		else if( task->opcode == OC_NUM_OPS )
		{
			res = rtable_num_ops(thread->rtable);
			struct quorum_op_t *re = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
			re->opcode = OC_NUM_OPS;
			re->id = task->id;
			re->sender = thread->id;
			re->content.result = res;
			//printf("THREAD %d METE RESPOSTA: %d\n", res->content.result);
			q_put_task(thread->out_queue, re);
		}
		else if( task->opcode == OC_DEL )
		{
			res = rtable_del( thread->rtable, task->content.key );
			thread->state = 1;
			reply = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
			reply->content.result = res;
			reply->id = task->id;
			reply->opcode = OC_DEL;
			reply->sender = thread->id;
			q_put_task(thread->out_queue, reply);
		}
	}

	return NULL;
}

/*
	Refresh active servers
*/
int refresh_active_servers(struct rtable_t **rtable){

	int i;

	//lanca threads
	//thread_launcher( &server_access, num_threads, threads_data );
	for( i = 0; i < num_threads; i++ )
	{
		if( ( pthread_create( &threads_data[i]->thread, NULL, &server_access, threads_data[i] ) ) != 0 )
		{
			perror("Erro ao lancar a thread");
		}
		pthread_detach(threads_data[i]->thread);
	}


	//cria pedido de size
	struct quorum_op_t *req = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
	req->opcode = OC_SIZE;
	req->sender = -1;

	//insere pedidos em files de cada thread
	for( i = 0; i < num_threads; i++){
		//sleep(4);
		//printf("MAIN INSERE EM %d\n", i);
		q_put_task(threads_data[i]->in_queue, req);
	}

	
	//retira pedidos de fila de respostas
	int inserted = 0;
	for( i = 0; i < num_threads; i++ )
	{	
		struct quorum_op_t *reply = (struct quorum_op_t *) q_get_task(replies_queue);
		//puts("MAIN OBTEVE");

		//insere em activas
		if( inserted < num_active_threads )
		{
			int j;
			for( j = 0; j < num_threads; j++ )
			{	
				if( threads_data[j]->id == reply->opcode )
				{
					active_threads[inserted] = threads_data[j];
					//puts("DEFINIU ACTIVA");
					inserted++;
				}
			}
		}
	}

	//conta servers activos
	int soma = 0;
	
	for( i = 0; i < num_active_threads; i++ )
	{
		if(active_threads[i] != NULL)
			soma++;
	}

	//inseriu em todas
	//printf("NUMERO DE ACTIVAS: %d\n", soma);
	
	return soma;
}

/* Função que envia um pedido a um conjunto de servidores e devolve
 * um array com o número esperado de respostas.
 * O parâmetro request é uma representação do pedido, enquanto
 * expected_replies representa a quantidade de respostas esperadas antes
 * da função retornar.
 * Note que os campos id e sender em request serão preenchidos dentro da
 * função. O array retornado é um array com k posições (0 a k-1), sendo cada
 * posição correspondente a um apontador para a resposta de um servidor
 * pertencente ao quórum que foi contactado.
 * Caso não se consigam respostas do quórum mínimo, deve-se retornar NULL.
 */
struct quorum_op_t **quorum_access(struct quorum_op_t *request, int expected_replies) {

	if(request == NULL)
		return NULL;
	int i;
	int max_i;
	long long max_ts;

	//reavalia se o quorum ainda se mantem, senao actualiza
	struct quorum_op_t **replies = send_timestamp_until_k(request);
	if( replies == NULL )
		return NULL;

	struct quorum_op_t **response = (struct quorum_op_t **) malloc(sizeof(struct quorum_op_t *) * num_active_threads);
	for( i = 0; i < num_active_threads; i++ )
		response[i] = NULL;
	
	//PUT
	if( request->opcode == OC_PUT )
	{
		//pede timestamps ao quorum desta entry->keys
		//obtem posicao do server activo com maior timestamp
		max_ts = -2;
		int m_pos;
		for(i = 0; i < num_active_threads; i++)
		{
			if( replies[i]->content.timestamp > max_ts )
			{
				max_ts = replies[i]->content.timestamp;
				m_pos = i;
			}
		}

		int put_result = rtable_put(threads_data[replies[m_pos]->sender]->rtable, request->content.entry->key, request->content.entry->value);
				
		
		max_ts = request->content.entry->value->timestamp;
		int actualizados = 0;
		for(i = 0; i < num_active_threads; i++)
		{
			if( replies[i]->sender != replies[m_pos]->sender )
			{

				//se conseguiu fazer put em mais recent
				if(put_result >= 0)
				{

					if( replies[i]->content.timestamp < max_ts && replies[i]->content.timestamp > 0 )
					{
						//request->content.entry->value->timestamp = max_ts;
						//rtable_update(threads_data[replies[i]->sender]->rtable, request->content.entry->key, request->content.entry->value);
						struct quorum_op_t *wb_req = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
						//se inseriuuuuuuuuu
						wb_req->opcode = OC_PUT;
						wb_req->content.entry = request->content.entry;
						q_put_task(threads_data[replies[i]->sender]->in_queue, wb_req);
						actualizados++;
					}
					else if( replies[i]->content.timestamp < 0  )
					{
						
						//request->content.entry->value->timestamp = max_ts;
						//rtable_put(threads_data[replies[i]->sender]->rtable, request->content.entry->key, request->content.entry->value);
						struct quorum_op_t *wb_req = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
						//se inseriu
						wb_req->opcode = OC_PUT;
						wb_req->content.entry = request->content.entry;
						q_put_task(threads_data[replies[i]->sender]->in_queue, wb_req);
						actualizados++;
					}	

				}

				
			}
			
		}
		for(i = 0; i < actualizados; i++)
		{
				
				
				response[i] = q_get_task(replies_queue);
				
				//se server timeout
			
		}

		

		if(put_result < 0)
		{
			response[0] = NULL;
			return response;
		}
		else
		{
			struct quorum_op_t **r = (struct quorum_op_t **) malloc(sizeof(struct quorum_op_t *) * num_active_threads);
			return r;
		}
	}
	//UPDATE
	else if( request->opcode == OC_UPDATE )
	{
		//obtem o maximo dos timestamps em replies
		int max_i = 0;
		max_ts = -1;
		for(i = 0; i < num_active_threads; i++)
		{
			if( replies[i] != NULL && replies[i]->content.timestamp > max_ts )
			{
				
				max_ts = replies[i]->content.timestamp;
				max_i = i;
			}
		}
		//executa update para mais recente
		
		

		//mete pedido em principal
		struct quorum_op_t **respostass = (struct quorum_op_t **) malloc(sizeof(struct quorum_op_t *) * num_active_threads);
		q_put_task( threads_data[replies[max_i]->sender]->in_queue, request );

		//recebe resposta de principal
		respostass[0] = q_get_task(replies_queue);

		int update_result = respostass[0]->content.result;


		//se ok
		if( update_result != -1 )
		{	
			for(i = 0; i < num_active_threads; i++)
			{
				if( i != max_i )
				{
					struct quorum_op_t *wb_task = ( struct quorum_op_t * ) malloc (sizeof(struct quorum_op_t));
					wb_task->content.entry = entry_dup(request->content.entry);
					wb_task->id = request->id;
					// se este estah a < 0 faz put
					if( replies[i]->content.timestamp < 0 )
						wb_task->opcode = OC_PUT;
					else if( replies[i]->content.timestamp > 0 && replies[i]->content.timestamp < request->content.entry->value->timestamp )
						wb_task->opcode = OC_UPDATE;

					//mete em fila de tasks
					q_put_task( threads_data[replies[i]->sender]->in_queue, wb_task );
				}
			}
		}
		
		if( update_result != -1 )	
			// se este estah a > 0 e < max_ts
			for( i = 1; i < num_active_threads; i++ )
			{
				int ok = 0;
				while( !ok )
				{
					respostass[i] = q_get_task(replies_queue);
					if( respostass[i]->id = request->id )
						ok = 1;
				}
			}

		return respostass;
	}
	//SIZE
	else if( request->opcode == OC_SIZE )
	{
		struct quorum_op_t **respostasas = (struct quorum_op_t **) malloc(sizeof(struct quorum_op_t *) * num_active_threads);

		struct quorum_op_t *req_num_ops = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t *));
		req_num_ops->id = request->id;
		req_num_ops->opcode = OC_NUM_OPS;

		//faz pedidos
		for(i = 0; i < num_active_threads; i++)
		{
			q_put_task(active_threads[i]->in_queue, req_num_ops);
		}

		//obtem respostas
		for( i = 0; i < num_active_threads; i++ )
		{	
			int ok = 0;
			while( !ok )
			{
				respostasas[i] = q_get_task(replies_queue);
				if( respostasas[i]->id == request->id )
					ok = 1;
			}
		}

		
		//obtem resposta com mais operacoes
		int num_ops;
		max_i = 0;
		for( i = 0; i < num_active_threads; i++ )
		{
			if( respostasas[i]->content.result > num_ops )
			{
				num_ops = respostasas[i]->content.result;
				max_i = i;
			}
		}
		

		//faz pedido size ao max_i
		int size = rtable_size(threads_data[respostasas[max_i]->sender]->rtable);
		
		
		respostasas[0]->content.result = size;
		return respostasas;
	}
	else if( request->opcode == OC_GET )
	{
		//se quer todas as keys
		if( (strcmp(request->content.key, "!")) == 0 || (strcmp(request->content.key, "!\n")) == 0 )
		{
			

			//obtem numero de operaçoes de servers no quorum
			struct quorum_op_t *num_opss = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
			num_opss->id = request->id;
			num_opss->opcode = OC_NUM_OPS;
			for( i = 0; i < num_active_threads; i++ )
				q_put_task(active_threads[i]->in_queue, num_opss);

			//espera por respostas
			struct quorum_op_t **num_opss_rsp = (struct quorum_op_t **) malloc(sizeof(struct quorum_op_t * )* num_active_threads);
			for(i = 0; i < num_active_threads; i++)
			{
				int ok = 0;
				while(!ok)
				{
					num_opss_rsp[i] = q_get_task(replies_queue);
					if( num_opss_rsp[i]->id == request->id )
						ok = 1;
				}
			}


			int max_ops = 0; 
			int max_ops_i = 0;
			for( i = 0; i < num_active_threads; i++ )
			{
				if( num_opss_rsp[i]->content.result > max_ops )
				{
					max_ops = num_opss_rsp[i]->content.result;
					max_ops_i = i;
				}
			}

			
			char **keys = rtable_get_keys(threads_data[num_opss_rsp[max_ops_i]->sender]->rtable);
			struct quorum_op_t **rsrs = (struct quorum_op_t **) malloc(sizeof( struct quorum_op_t * )* num_active_threads);
			rsrs[0] = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
			rsrs[0]->content.keys = keys;
			
			return rsrs;

		}
		//se quer apenas uma key
		else
		{
			
			int i;
			for( i = 0; i < num_active_threads; i++ )
				q_put_task(active_threads[i]->in_queue, request);

			struct quorum_op_t **respostas = (struct quorum_op_t **) malloc(sizeof(struct quorum_op_t*) * num_active_threads);
			for( i = 0; i < num_active_threads; i++ )
			{
				int ok = 0;
				while( !ok )
				{
					respostas[i] = q_get_task(replies_queue);
					if( respostas[i]->id == request->id && respostas[i]->opcode == OC_GET )
						ok = 1;
				}
				
			}

			long long max_ts2 = -1;
			max_i = -1;
			for( i = 0; i < num_active_threads; i++ )
			{
				
				if( respostas[i] != NULL && respostas[i]->content.data != NULL )
				{
					
					if( respostas[i]->content.data->timestamp > max_ts2 )
					{
						max_i = i;
						max_ts2 = respostas[i]->content.data->timestamp;
						
					}
				}
			}

			//se nao existe em nenhum dos servers
			if( max_i < 0 )
			{
				respostas[0]->content.data = data_create(0);
				respostas[0]->content.data->datasize = -1;
				return respostas;
			}
				

			puts("");
			
			
			/*for( i = 0; i < num_active_threads; i++ )
			{
				if(respostas[i] == NULL)
					printf("THREAD %d nao tem a  entry -> NULL\n", respostas[i]->sender);
				else if(respostas[i]->content.data == NULL)
					printf("THREAD %d nao tem a  entry -> DATA A NULL\n", respostas[i]->sender);
				else if( respostas[i] != NULL && respostas[i]->content.data != NULL ){
					printf("VALUE: %s\n", respostas[i]->content.data->data);
					printf("TIMESTAMP: %lld\n", respostas[i]->content.data->timestamp);
				}
			}*/
			
			// se deve actualizar para null
			for( i = 0; i < num_active_threads; i++ )
			{	
				if( i != max_i )
				{
					//se deve inserir a null
					if( respostas[i]->content.data == NULL )
					{
						rtable_put(threads_data[respostas[i]->sender]->rtable, request->content.key, respostas[max_i]->content.data);
					}
					else if( respostas[i]->content.data->timestamp < max_ts2 )
					{
						rtable_update(threads_data[respostas[i]->sender]->rtable, request->content.key, respostas[max_i]->content.data);
					}
				}
			}
			
			return respostas;
		}
	}
	else if( request->opcode == OC_RT_GETTS )
	{
		return replies;
	}
	return NULL;
}

/* Liberta a memoria, destroi as rtables usadas e destroi as threads.
 */
int destroy_quorum_access() {
	
	int i;
	struct qnode_t *head;
	struct q_head_t *queue;

	for( i = 0; i < num_threads; i++ )
	{
		queue = queues[i];
		head = queue->queue_head;
		while( head != NULL )
		{
			free_quorum_op_t(head->task);
			head->next;
		}
	}

	return 0;
}

///////////////////////////////////////////////
/* garante no minimo k respostas */
struct quorum_op_t **send_timestamp_until_k( struct quorum_op_t *request )
{
	
	int i, k_rsp = 0;
	struct quorum_op_t **replies;
	int max_tries = 3;
	int try = 0;
	while( k_rsp != num_active_threads && try < max_tries ){
		replies = send_get_timestamp_to_k(request);
		
		k_rsp = 0;
		for( i = 0; i < num_active_threads; i++)
			if(replies[i] != NULL)
				k_rsp++;

		//se diferente faz free de replies
		if( k_rsp != num_active_threads)
			for( i = 0; i < num_active_threads; i++ )
				free(replies[i]);
		
		try++;

	}
	
	if( k_rsp != num_active_threads )
		return NULL;

	return replies;
}
/* envia get_ts para k servers */
struct quorum_op_t **send_get_timestamp_to_k(struct quorum_op_t *request){

	//pthread_mutex_init(&threads_data[0]->in_queue->queue_lock, NULL);
	int i;

	//////////////////////////////////////////////////////////////////////
	// TIMESTAMP
	struct quorum_op_t **ts_replies = (struct quorum_op_t **) malloc(sizeof(struct quorum_op_t) * num_active_threads);
	for( i = 0; i < num_active_threads; i++ )
		ts_replies[i] = NULL;

	struct quorum_op_t *req_time;
	if( request->opcode == OC_PUT || request->opcode == OC_UPDATE)
		req_time = create_quorum_op_ts(request->id, request->content.entry->key);
	else if( request->opcode == OC_RT_GETTS || request->opcode == OC_DEL )
		req_time = create_quorum_op_ts(request->id, request->content.key);
	else
		req_time = create_quorum_op_ts(request->id, "");


	for(i = 0; i < num_active_threads; i++)
		q_put_task(active_threads[i]->in_queue, req_time);

	
	//aguarda por respostas de timestamps
	for(i = 0; i < num_active_threads; i++){
		
		int ok = 0;

		//considera apenas respostas do pedido actual
		while( !ok ){
			ts_replies[i] = q_get_task(replies_queue);
			if( ts_replies[i] == NULL || ts_replies[i]->id == request->id )
				ok = 1;
		}

		//verifica se deu timeout
		if( ts_replies[i] == NULL )
		{
			//se ainda existe um activo diferente dos jah activos
			int new_active_pos = get_available_server();
			
			if( new_active_pos != -1  )
			{	
				
				//se fez bem a substituição por um activo
				if( (replace_inactive_server( new_active_pos )) != -1 )
				{
					
					q_put_task(threads_data[new_active_pos]->in_queue, req_time);
					i--;
				}
			}

		}

		//printf("TIMESTAMP: %lld\n", ts_replies[i]->content.timestamp);
	}
	
	int count = 0;
	for( i = 0; i < num_active_threads; i++ )
	{
		if( ts_replies[i] != NULL )
			count++;
	}

	//se nao obteve no minimo uma resposta sai do metodo
	if( count == 0 ){
		return NULL;
	}

	return ts_replies;
}


///////////////////////////////////////////////
//replace servers
int replace_inactive_server(int active_pos){

	int i;
	for( i = 0; i < num_active_threads; i++ )
	{
		if( active_threads[i]->rtable->server->is_connected == -1 )
		{
			
			active_threads[i] = threads_data[active_pos];
			return 0;
		}
	}

	return -1;
}
int get_available_server(){

	int i, j;

	for( i = 0; i < num_threads; i++ )
	{
		
		if( threads_data[i]->rtable->server->is_connected != -1 )
		{
			
			//verifica se nao esta em activos
			for( j = 0; j < num_active_threads; j++ )
			{	
				//se jah esta em activas continua a procura
				if( threads_data[i]->id == active_threads[j]->id )
					break;
				//
				if( threads_data[i]->id != active_threads[j]->id && j == (num_active_threads-1) ){
					return i;
				}
			}
		}
	}

	return -1;
}

///////////////////////////////////////////////
// create quorum_op_t
struct quorum_op_t *create_quorum_op_ts(int id, char *key)
{
	struct quorum_op_t *q = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
	q->opcode = OC_RT_GETTS;
	q->id = id;
	q->content.key = key;
	return q;
}


//free de quorum_op_t
void free_quorum_op_t(struct quorum_op_t *q)
{	
	if( q == NULL)
		return;
	switch( q->opcode )
	{
		case(OC_PUT):
			entry_destroy(q->content.entry);
		break;
		case(OC_UPDATE):
			entry_destroy(q->content.entry);
		break;
		case(OC_GET):
			if( strcmp( q->content.key, "!" ) == 0 || strcmp( q->content.key, "!\n" ) == 0 )
			{
				qtable_free_keys(q->content.keys);
			}
			else
				free(q->content.key);
		break;
		case(OC_DEL):
			free(q->content.key);
		break;
	}

	free(q);
}




