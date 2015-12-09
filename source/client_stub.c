/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "inet.h"
#include "message-private.h"
#include "client_stub-private.h"
#include "network_client-private.h"

struct rtable_t *rtable_bind(const char *address_port){

	if( address_port == NULL )
		return NULL;

	struct rtable_t *remote_table = (struct rtable_t *) malloc( sizeof( struct rtable_t ) );
	if( remote_table == NULL )
		return NULL;
	remote_table->server = network_connect(address_port);
	if( remote_table->server == NULL){
		return NULL;
	}
	return remote_table;
}


int rtable_unbind(struct rtable_t *rtable){
	if( rtable == NULL )
		return -1;

	network_close(rtable->server);
	free(rtable);
	return 0;
}


int rtable_put(struct rtable_t *rtable, char *key, struct data_t *value){
	if( rtable == NULL || key == NULL )
		return -1;

	// cria request
	struct message_t *request = (struct message_t *) malloc(sizeof( struct message_t ));

	if( request == NULL )
		return -1;

	request->opcode        = OC_PUT;
	request->c_type        = CT_ENTRY;
	request->content.entry = entry_create(key, value);

	// faz pedido ao server
	struct message_t *response = network_send_receive(rtable->server, request);
	int res = 0;

	// se nao recebeu correctamente a resposta
	if( response == NULL )
	{

		res = -1;
	}
	// se recebeu correctamente a resposta do servidor
	else
	{
		if (response->opcode == OC_RT_ERROR)
		{
			//liberta memoria de response
			free_message(response);
			return -1;
		}

		//liberta memoria de response
		free_message(response);
	}

	//liberta memoria utilizada
	free_message(request);

	return res;
}


int rtable_update(struct rtable_t *rtable, char *key, struct data_t *value){
	if( rtable == NULL || key == NULL || value == NULL )
		return -1;

	// cria request
	struct message_t *request = (struct message_t *) malloc(sizeof( struct message_t ));
	if( request == NULL )
		return -1;

	request->opcode = OC_UPDATE;
	request->c_type = CT_ENTRY;
	request->content.entry = entry_create(key, value);

	// faz pedido a servidor
	struct message_t *response = network_send_receive(rtable->server, request);

	// se nao recebeu correctamente a resposta
	if( response == NULL )
	{
		//liberta memoria utilizada
		free_message(request);
		free_message(response);

		return -1;
	}
	// se recebeu correctamente a resposta do servidor
	else
	{
		if( response->opcode == OC_RT_ERROR )
		{
			return -1;
		}
	}

	//liberta memoria utilizada
	free_message(request);
	free_message(response);

	return 0;
}


struct data_t *rtable_get(struct rtable_t *table, char *key){

	if( table == NULL || key == NULL )
		return NULL;

	struct message_t *request = (struct message_t *) malloc(sizeof(struct message_t));

	if( request == NULL )
		return NULL;

	request->opcode = OC_GET;
	request->c_type = CT_KEY;
	request->content.key = strdup(key);

	// faz pedido ao servidor
	struct message_t *response = network_send_receive(table->server, request);

	// se nao recebeu correctamente a resposta
	if( response == NULL )
	{
		//liberta memoria utilizada
		free_message(request);

		return NULL;
	}
	// se recebeu correctamente a resposta do servidor
	else if( response->opcode == OC_RT_ERROR )
	{
		free_message(response);
		free_message(request);

		return NULL;
	}

	struct data_t *data;

	//devolver todas as keys
	if (strcmp(request->content.key, "!") == 0) {
	//devolver data referente ah key
	} else {
		data = data_create2( response->content.data->datasize, response->content.data->data );
		data->timestamp = response->content.data->timestamp;
		/*printf("Key: %s | Data: %s | Timestamp: %lld\n", 
			request->content.key, (char*) response->content.data->data,
			response->content.data->timestamp);
			*/
	}

	// liberta memoria
	free_message(request);
	free_message(response);

	return data;
}


int rtable_del(struct rtable_t *table, char *key){
	if( table == NULL || key == NULL )
		return -1;

	struct message_t *request = (struct message_t *) malloc(sizeof(struct message_t));

	if( request == NULL )
		return -1;

	request->opcode = OC_DEL;
	request->c_type = CT_KEY;
	request->content.key = strdup(key);

	// faz pedido ao servidor
	struct message_t *response = network_send_receive(table->server, request);
	int result_code = 0;

	// se nao recebeu correctamente a resposta
	if( response == NULL )
	{
		//liberta memoria utilizada
		result_code = -1;
	}
	// se recebeu correctamente a resposta do servidor
	else if( response->opcode == OC_RT_ERROR )
	{
		result_code = -1;
	}

	free_message(request);
	free_message(response);

	return result_code;
}


int rtable_size(struct rtable_t *rtable){
	if(rtable == NULL)
		return -1;

	int size = 0;
	struct message_t *request = (struct message_t *) malloc(sizeof(struct message_t));
	if( request == NULL )
		return -1;
	request->opcode = OC_SIZE;
	request->c_type = CT_RESULT;
	request->content.result = 200;

	//puts("RTABLE SIZE");
	//printf("Server State: %d\n", rtable->server->is_connected);
	//faz pedido ao servidor
	struct message_t *response = network_send_receive(rtable->server, request);

	// se nao recebeu correctamente a resposta
	if( response == NULL )
	{
		//liberta memoria utilizada
		free_message(request);
		free_message(response);
		return -1;
	}
	// se recebeu correctamente a resposta do servidor
	else
	{
		// se inseriu entry
		if( response->opcode == OC_SIZE + 1 )
		{
			//puts("\nSize obtido!");
			size = response->content.result;
		}
	}

	free_message(request);
	free_message(response);

	return size;
}


int rtable_num_ops(struct rtable_t *rtable){
	if(rtable == NULL)
		return -1;

	int num_ops = 0;
	struct message_t *request = (struct message_t *) malloc(sizeof(struct message_t));
	if( request == NULL )
		return -1;
	request->opcode = OC_NUM_OPS;
	request->c_type = CT_RESULT;
	request->content.result = 200;

	//puts("RTABLE SIZE");
	//printf("Server State: %d\n", rtable->server->is_connected);
	//faz pedido ao servidor
	struct message_t *response = network_send_receive(rtable->server, request);
	// se nao recebeu correctamente a resposta
	if( response == NULL )
	{
		//liberta memoria utilizada
		free_message(request);
		free_message(response);
		return -1;
	}
	// se recebeu correctamente a resposta do servidor
	else
	{
		// se inseriu entry
		if( response->opcode == OC_NUM_OPS + 1 )
		{
			//puts("\nSize obtido!");
			num_ops = response->content.result;
		}
	}

	free_message(request);
	free_message(response);

	return num_ops;
}



char **rtable_get_keys(struct rtable_t *rtable){
	if( rtable == NULL )
		return NULL;

	char **keys = NULL;
	struct message_t *request =
		(struct message_t *) malloc(sizeof(struct message_t));

	if(request == NULL)
		return NULL;

	request->opcode = OC_GET;
	request->c_type = CT_KEY;
	request->content.key = strdup("!");

	// faz pedido ao servidor
	struct message_t *response =
		network_send_receive(rtable->server, request);

	// se nao recebeu correctamente a resposta
	if( response == NULL )
	{

		//liberta memoria utilizada
		free_message(request);
		free_message(response);

		return NULL;
	}
	// se recebeu correctamente a resposta do servidor
	else
	{
		// se inseriu entry
		if( response->opcode == OC_GET + 1 )
		{
			int i, size = 0;

			for (i = 0; response->content.keys[i] != NULL; i++)
			{
				size++;
			}

			//criar array
			keys = (char **) malloc( sizeof(char *) * (size + 1) );

			for (i = 0; response->content.keys[i] != NULL; i++)
			{
				keys[i] = strdup( response->content.keys[i] );
			}

			keys[i] = NULL;
		}
	}

	free_message(request);
	free_message(response);

	if( keys == NULL )
		return NULL;

	return keys;
}


void rtable_free_keys(char **keys){
	if( keys == NULL )
		return;

	int i = 0;
	for (i = 0; keys[i] != NULL ; ++i)
	{
		free(keys[i]);
	}
	free(keys);
}


long long rtable_get_ts(struct rtable_t *rtable, char *key){

	if( rtable == NULL || key == NULL )
		return -1;

	struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
	msg->c_type = CT_KEY;
	msg->opcode = OC_RT_GETTS;
	msg->content.key = strdup(key);

	struct message_t *reply = network_send_receive(rtable->server, msg);
	return reply->content.timestamp;
}


int rtable_get_numops(struct rtable_t *rtable){

	if( rtable == NULL)
		return -1;

	struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
	msg->content.result = 0;
	msg->c_type         = CT_RESULT;
	msg->opcode         = OC_NUM_OPS;

	struct message_t *reply = network_send_receive(rtable->server, msg);

	return reply->content.timestamp;
}
