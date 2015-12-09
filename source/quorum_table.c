#include <stdlib.h>
#include <string.h>

#include "quorum_table-private.h"
#include "client_stub-private.h"
#include "network_client-private.h"
#include "entry.h"
#include "inet.h"
#include "table-private.h"
#include "data-private.h"
#include "quorum_access.h"

/* Define os possíveis opcodes da mensagem */
#define OC_SIZE	10
#define OC_DEL		20
#define OC_UPDATE 30
#define OC_GET		40
#define OC_PUT		50


/* Função para estabelecer uma associação entre uma tabela qtable_t e
 * um array de n servidores.
 * addresses_ports é um array de strings e n é o tamanho deste array.
 * Retorna NULL caso não consiga criar o qtable_t.
 */
struct qtable_t *qtable_bind(const char **addresses_ports, int n){

	if( addresses_ports == NULL )
		return NULL;

	struct qtable_t *qtable = ( struct qtable_t *) malloc( sizeof( struct qtable_t ) );
	if( qtable == NULL )
		return NULL;
	
	struct rtable_t *rtables = (struct rtable_t *) malloc( sizeof(struct rtable_t) * n );
	qtable->rtables = rtables;
	qtable->num_tables = n;
	int min =  floor( n / 2 ) + 1;
	qtable->id = 1;

	//inicia as tables remotas
	int i;
	int error = 0;
	for( i = 0; i < n; i++){
		struct rtable_t *aux = rtable_bind(addresses_ports[i + 2]);
		if( aux == NULL )
		{	
			error++;
		}
		else
		{
			qtable->rtables[i] = *aux;
			printf("Connectado a server nº %d\n", i + 1);
		}
	}
	if(error >= min){
		return NULL;
	}
	//inicia quorum de servers
	init_quorum_access(qtable->rtables, n);

	return qtable;
}


/* Fecha a ligação com os servidores do sistema e liberta a memória alocada
 * para qtable. 
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int qtable_disconnect(struct qtable_t *qtable){

	if( qtable == NULL )
		return -1;
	destroy_quorum_access();
	/*
	// liberta memoria de tabelas remotas	
	int max;
	max = qtable->num_tables;
	for( i = 0; i < max; i++ )
	{
		int unbind_code = rtable_unbind( qtable->rtables[i] );
		if( unbind_code == -1 )
			printf("Erro ao fazer unbind de remote table %i\n", unbind_code);
		else
			printf("Remote table %i unbinded!\n", unbind_code);
	}
	*/
	//liberta memoria de qtable
	free( qtable );

	return 0;
}


/* Função para adicionar um elemento na tabela.
 * Note que o timestamp em value será atribuído internamente a esta função,
 * como definido no algoritmo de escrita.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int qtable_put(struct qtable_t *qtable, char *key, struct data_t *value){

	if( qtable == NULL || key == NULL )
		return -1;
	

	qtable->operation++;

	//formula request de timestamps
	struct quorum_op_t *request = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
	request->id = qtable->operation;
	request->opcode = OC_RT_GETTS;
	request->content.key = key;

	int expected_replies = floor( qtable->num_tables / 2 ) + 1;

	struct quorum_op_t **timestamps = quorum_access(request, expected_replies);
	if( timestamps == NULL )
		return -2;

	int i;
	long long max_ts = -1;
	for( i = 0; i < expected_replies; i++ )
	{
		if( timestamps[i] != NULL && timestamps[i]->content.timestamp != 0 && timestamps[i]->content.timestamp > max_ts)
		{
			max_ts = timestamps[i]->content.timestamp;
		}
	}
	

	int result = 0;
	request = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
	if( request == NULL )
		result = -1;
	else{
		//incrementa a operacao
		qtable->operation++;
		request->id = qtable->operation;
		request->opcode = OC_PUT;			//atribuir o mesmo timestamp a value

		int result = 0;
		//se nao tem nenhuma entry com esse valor
		if( max_ts == -1 )
		{
		
			int expected_replies = floor(qtable->num_tables / 2) + 1;
			value->timestamp = inc_timestamp(0, qtable->id);
			request->content.entry = entry_create(key, value);	
			struct quorum_op_t **replies = quorum_access(request, expected_replies);

			if( replies == NULL || replies[0] == NULL )
				result = -1;
		}
		//se existirem entries obtem mais recent, 
		//faz get e so se for null eq que faz update
		else
		{
			long long new_ts = inc_timestamp(max_ts, 1);
			value->timestamp = new_ts;
			request->content.entry = entry_create(key, value);	
			struct quorum_op_t **replies = quorum_access(request, expected_replies);

			if( replies == NULL )
				result = -2;
			else if( replies[0] == NULL )
				result = -1;
			

		}
	}
	
	// DESTROY all_ts here
	for( i = 0; i < expected_replies; i++ )
		free(timestamps[i]);

	return result;
}

/* Função para atualizar o valor associado a uma chave.
 * Note que o timestamp em value será atribuído internamente a esta função,
 * como definido no algoritmo de update.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int qtable_update(struct qtable_t *qtable, char *key,
                  struct data_t *value){
	
	qtable->operation++;

	//formula request de timestamps
	struct quorum_op_t *request = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
	request->id = qtable->operation;
	request->opcode = OC_RT_GETTS;
	request->content.key = key;

	int expected_replies = floor( qtable->num_tables / 2 ) + 1;

	struct quorum_op_t **timestamps = quorum_access(request, expected_replies);
	if( timestamps == NULL )
		return -2;

	int i;
	long long max_ts = -1;
	for( i = 0; i < expected_replies; i++ )
	{
		
		if( timestamps[i] != NULL && timestamps[i]->content.timestamp != 0 && timestamps[i]->content.timestamp > max_ts)
		{
			
			max_ts = timestamps[i]->content.timestamp;
		}
	}

	//se nao tem nenhuma entry com esse valor
	if( max_ts == -1 )
	{
		return -1;
	}


	//se existe incrementa o seu timestamp
	max_ts = inc_timestamp(max_ts, qtable->id);
	
	// DESTROY all_ts here
	for( i = 0; i < expected_replies; i++ )
		free(timestamps[i]);

	//incrementa numero de operacao
	qtable->operation++;

	//formula pedido de update
	request->id = qtable->operation;
	request->opcode = OC_UPDATE;
	value->timestamp = max_ts;
	request->content.entry = entry_create(key, value);

	//faz pedido de update ao quorum
	struct quorum_op_t **update_rsp = quorum_access(request, expected_replies);
	if( update_rsp == NULL )
		return -1;
	//se algum deu resposta entao estah ok e devolve 0
	return update_rsp[0]->content.result;
}

/* Função para obter um elemento da tabela.
 * Em caso de erro ou elemento não existente, devolve NULL.
 */
struct data_t *qtable_get(struct qtable_t *qtable, char *key){
		
	qtable->operation++;

	//formula o pedido de get para quorum
	struct quorum_op_t *request = (struct quorum_op_t*) malloc(sizeof(struct quorum_op_t));
	request->content.key = key;
	request->opcode = OC_GET;
	request->id = qtable->operation;

	//calcula o numero esperado de respostas
	int expected = (floor( qtable->num_tables / 2 ) + 1);

	//faz pedido a quorum
	struct quorum_op_t **response = quorum_access(request, expected);
	if( response == NULL )
	{
		
		return NULL;
	}
	
	if( response[0] != NULL && response[0]->content.data != NULL && response[0]->content.data->datasize == -1 )
	{	
		
		return response[0]->content.data;
	}
	//eleiçao de resposta mais recente
	long long ts = -2;
	int i, pos, exists;
	for( i = 0; i < expected; i++ )
	{
		if( response[i] != NULL )
			if( response[i]->content.data != NULL && response[i]->content.data->timestamp > ts )
			{
				ts = response[i]->content.data->timestamp;
				pos = i;
				exists = 1;
			}
	}
	
	struct data_t *data = NULL;

	//se obteve resposta
	if( response[pos] != NULL && exists == 1 )
	{	

		data = data_dup(response[pos]->content.data);
	}
	

	//free respostas	
	for( i = 0; i < expected; i++ )
	{
		if( response[i] != NULL ){
			if( response[i]->content.data != NULL )
				data_destroy(response[i]->content.data);
			
			free(response[i]);
		}
	}

	//se nenhum deu resposta
	return data;
}

/* Função para remover um elemento da tabela. É equivalente à execução 
 * put(k, NULL) se a chave existir. Se a chave não existir, nada acontece.
 * Devolve 0 (ok), -1 (chave não encontrada).
 */
int qtable_del(struct qtable_t *qtable, char *key){

	qtable->operation++;

	//formula request de timestamps
	struct quorum_op_t *request = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
	request->id = qtable->operation;
	request->opcode = OC_RT_GETTS;
	request->content.key = key;

	int expected_replies = floor( qtable->num_tables / 2 ) + 1;

	struct quorum_op_t **timestamps = quorum_access(request, expected_replies);
	if( timestamps == NULL )
		return -2;

	int i;
	long long max_ts = -1;
	for( i = 0; i < expected_replies; i++ )
	{
		if( timestamps[i] != NULL && timestamps[i]->content.timestamp != 0 && timestamps[i]->content.timestamp > max_ts)
		{
			max_ts = timestamps[i]->content.timestamp;
		}
	}

	//se nao tem nenhuma entry com esse valor
	if( max_ts == -1 )
		return -1;

	
	struct data_t *data = qtable_get(qtable, key);
	if( data == NULL || data->data == NULL )
		return -1;
		


	//se existe incrementa o seu timestamp
	max_ts = inc_timestamp(max_ts, qtable->id);
	
	// DESTROY all_ts here
	for( i = 0; i < expected_replies; i++ )
		free(timestamps[i]);

	//incrementa numero de operacao
	qtable->operation++;
	struct data_t *value = data_create(0);
	//formula pedido de update
	request->id = qtable->operation;
	request->opcode = OC_UPDATE;
	value->timestamp = max_ts;
	request->content.entry = entry_create(key, value);

	//faz pedido de update ao quorum
	struct quorum_op_t **update_rsp = quorum_access(request, expected_replies);
	if( update_rsp == NULL )
		return -1;
	//se algum deu resposta entao estah ok e devolve 0
	return update_rsp[0]->content.result;

	return 0;
}

/* Devolve número (aproximado) de elementos da tabela ou -1 em caso de
 * erro.
 */
int qtable_size(struct qtable_t *qtable){

	qtable->operation++;
	//formula pedido de get num opts
	struct quorum_op_t *req = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
	req->id = qtable->operation;
	req->opcode = OC_SIZE;
	//calcula numero de respostas esperadas
	int expected = floor(qtable->num_tables / 2) + 1;

	//faz pedido ao server
	struct quorum_op_t **replies = quorum_access(req, expected);
	if( replies == NULL )
		return -2;
	
	if( replies != NULL && replies[0] != NULL )
		return replies[0]->content.result;

	return -1;
}

/* Devolve um array de char* com a cópia de todas as keys da tabela,
 * e um último elemento a NULL. Esta função não deve retornar as
 * chaves removidas, i.e., a NULL.
 */
char **qtable_get_keys(struct qtable_t *qtable){

	//formula pedido de get get num opts
	qtable->operation++;
	struct quorum_op_t *req = (struct quorum_op_t *) malloc(sizeof(struct quorum_op_t));
	req->id = qtable->operation;
	req->opcode = OC_GET;
	req->content.key = strdup("!");
	//calcula numero de respostas esperadas
	int expected = floor(qtable->num_tables / 2) + 1;

	//faz pedido ao server
	struct quorum_op_t **rep = quorum_access(req, expected);
	if( rep == NULL )
		return NULL;

	//devole o array de keys
	return rep[0]->content.keys;
}


/* Liberta a memória alocada por qtable_get_keys().
 */
void qtable_free_keys(char **keys){
	
	int i;
	for (i = 0; keys[i] != NULL; i++) {
		free(keys[i]);
	}

	free(keys);

}







