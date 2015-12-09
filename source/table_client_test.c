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
#include "message.h"
#include "data-private.h"
#include "network_client-private.h"
#include "table_client_test-private.h"
#include "client_stub-private.h"

/*
 * A ação associada à receção de SIGPIPE passa a ser ignorada.
 */
int ignsigpipe(){
   struct sigaction s;

   s.sa_handler = SIG_IGN;
   return sigaction(SIGPIPE, &s, NULL);
}


int main(int argc, char **argv) {
	if (argc < 2) {
		puts("Numero de parametros errado!");
		puts("Exemplo de uso: ./table_client <ip>:<porto>");
		return -1;
	}

	//ignorar sinais do tipo SIGPIPE
   if (ignsigpipe() != 0){
      perror("ignsigpipe falhou");
      exit(1);
   }

	char *cmd = NULL;
	size_t max = MAX_MSG + 1;
	const char delim[2] = " ";
	char *host = strdup(argv[1]);

	//estabelece ligacao
	struct rtable_t *remote_table = rtable_bind(host);
	free(host);

	if ( remote_table == NULL || remote_table->server == NULL ) {
		perror("Erro ao estabelecer ligacao");
		return -1;
	} else {
		puts("Ligacao estabelecida!");
	}

	// string de comando a executar
	cmd = (char *) malloc(max);
	if (cmd == NULL)
		perror("Erro ao alocar memoria\n");


	do	{
		puts("\n- Lista de comandos -");
		puts("put <key> <value>");
		puts("get !");
		puts("get <key>");
		puts("update <key> <value>");
		puts("del <key>");
		puts("size");
		puts("timestamp <key>");
		puts("numops");
		puts("quit\n");
		puts("Inserir comando: ");

		// obtem input

		getline(&cmd, &max, stdin);
		char *token = strtok(cmd, delim);

		if( remote_table == NULL )
			break;

		// PUT
		if( isPUT(token) )
		{
			exec_put(remote_table, token, delim);
		}
		// GET
		else if( isGET(token) )
		{
			exec_get(remote_table, token, delim);
		}
		// DEL
		else if( isDEL(token) )
		{
			exec_del(remote_table, token, delim);
		}
		// UPDATE
		else if( isUPDATE(token) )
		{
			exec_update(remote_table, token, delim);
		}
		// SIZE
		else if( isSIZE(token) )
		{
			exec_size(remote_table);
		}
		//TIMESTAMP
		else if( isTimestamp(token) )
		{
			exec_timestamp(remote_table, token, delim);
		}
		//NUM_OPS
		else if( isNumOps(token) )
		{
			exec_numops(remote_table);
		}
		// QUIT
		else if( !isQUIT( token ) )
		{
			puts("Comando invalido!");
		}

		// clean token
		token_cleaner(token, delim);
		//free(token);
	}while( !isQUIT(cmd) && remote_table != NULL && remote_table->server->is_connected == 1 );

	rtable_unbind(remote_table);

	puts("Terminado");

	free(cmd);
	return 0;
}


// exec methods
void exec_put(struct rtable_t *rtable, char *token, const char *delim){

	// get key
	token = strtok(NULL, delim);

	if( token == NULL )
	{
		puts("\nComando put invalido!");
		puts("Exemplo de uso: put <key> <value>\n");
		return;
	}

	char *key = strdup(token);
	//get value
	const char valor[2] = "\n";
	token = strtok(NULL, valor);

	if( token == NULL )
	{
		puts("Comando put invalido!");
		puts("Exemplo de uso: put <key> <value>\n");
	}
	else{
		//cria data da entry
		char *value = strdup(token);
		long long timestamp = 5LL;
		struct data_t *data = data_create3(strlen(value) + 1, value, timestamp);
		printf("table_client_test.c -> timestamp %lld\n", data->timestamp);

		//passa ah api
		rtable_put(rtable, key, data);
		free(value);
		data_destroy(data);
	}

	free(key);
}


void exec_get(struct rtable_t *rtable, char *token, const char *delim){
	const char valor[2] = "\n";
	token = strtok(NULL, valor);

	if( token == NULL )
	{
		puts("\nComando get invalido!");
		puts("Exemplo de uso: get <key>\n");
		return;
	}

	//se sao todas as keys
	if( strcmp( "!", token ) == 0 || strcmp( "!\n", token ) == 0 )
	{
		char **keys = rtable_get_keys(rtable);

		if( keys != NULL )
		{
			puts("\nKeys encontradas:");
			int i = 0;

			for(i = 0; keys[i] != NULL; i++)
			{
				printf("Key %d: %s\n", i + 1, keys[i]);
			}

			rtable_free_keys(keys);
		}
	}
	// se eh apenas uma key
	else
	{
		struct data_t *data = rtable_get(rtable, token);

		// se obteve resposta
		if( data != NULL )
		{
			printf("Key: %s | Data: %s | Timestamp: %lld\n", 
				token, (char*) data->data, data->timestamp );

			data_destroy(data);
		}
		// se nao obteve resposta
		else {
			//se a connection foi fechada
			if( rtable->server->is_connected == -1 )
			{
				puts("Server terminou ligacao!");
			}
		}
	}
}


void exec_del(struct rtable_t *rtable, char *token, const char *delim){

	const char valor[2] = "\n";
	token = strtok(NULL, valor);

	if( token == NULL ){
		puts("\nComando del invalido!");
		puts("Exemplo de uso: del <key>\n");
		return;
	}

	struct data_t *value = data_create3(0, NULL, 555);

	if (value == NULL) {
		puts("Erro ao criar data");
		return;
	}

	int result_code = rtable_update(rtable, token, value);
	data_destroy(value);

	//se nao recebeu resposta
	if( result_code < 0 ){
		//se server fechou connection
		if( rtable->server->is_connected == -1 )
		{
			puts("Servidor terminou a ligacao!");
		}
	}
	else{
		//se foi eliminada
		puts("Entry eliminada!");
	}
}


void exec_update(struct rtable_t *rtable, char *token, const char *delim){
	// get key
	token = strtok(NULL, delim);

	if( token == NULL )
	{
		puts("\nComando update invalido!");
		puts("Exemplo de uso: update <key> <value>\n");
		return;
	}

	char *key = strdup(token);

	//get value
	const char valor[2] = "\n";
	token = strtok(NULL, valor);
	if( token == NULL )
	{
		puts("\nComando update invalido!");
		puts("Exemplo de uso: update <key> <value>\n");
		return;
	}

	//cria value
	char *value = strdup(token);
	long long timestamp = 5;
	struct data_t *data = data_create3(strlen(value) + 1, value, timestamp);

	// faz pedido
	int result_code = rtable_update(rtable, key, data);

	//se nao recebeu resposta
	if( result_code < 0 )
	{
		if( rtable->server->is_connected == -1 )
			puts("Servidor terminou ligacao");
		else
			printf("Entry nao actualizada\n");
	}

	//libertacao de memoria
	free(key);
	free(value);
	data_destroy(data);
}


void exec_size(struct rtable_t *rtable){
	if( rtable == NULL)
		return;

	int size = rtable_size(rtable);

	//se nao recebeu resposta
	if( size < 0 )
	{
		if( rtable->server->is_connected == -1 )
			puts("Servidor terminou a ligacao");
		else
			printf("Erro ao obter size.\n");
	}else{

		//se recebeu bem o size
		printf("Size: %d\n", size);

	}
}


void exec_timestamp(struct rtable_t *rtable, char *token, const char *delim) {
	if( rtable == NULL)
		return;

	// get key
	const char valor[2] = "\n";
	token = strtok(NULL, valor);

	if( token == NULL )
	{
		puts("\nComando update invalido!");
		puts("Exemplo de uso: update <key> <value>\n");
		return;
	}

	char *key = strdup(token);

	if (key == NULL)
	{
		puts("key NULL");
	}

	long long timestamp = rtable_get_ts(rtable, key);
	printf("timestamp: %lld\n", timestamp);

}


void exec_numops(struct rtable_t *rtable) {
	if( rtable == NULL)
		return;

	int numops = rtable_get_numops(rtable);
	printf("\nNumops: %d\n", numops);

}


/////////////////////////
// verifiers
int isPUT(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "put") == 0 ||
		strcmp(str, "put\n") == 0 ? 1 : 0;
}


int isGET(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "get") == 0 ||
		strcmp(str, "get\n") == 0 ? 1 : 0;
}


int isDEL(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "del") == 0 ||
		strcmp(str, "del\n") == 0 ? 1 : 0;
}


int isUPDATE(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "update") == 0 ||
		strcmp(str, "update\n") == 0 ? 1 : 0;
}


int isSIZE(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "size") == 0 ||
		strcmp(str, "size\n") == 0 ? 1 : 0;
}


int isTimestamp(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "timestamp") == 0 ||
		strcmp(str, "timestamp\n") == 0 ? 1 : 0;
}


int isNumOps(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "numops") == 0 ||
		strcmp(str, "numops\n") == 0 ? 1 : 0;
}


int isQUIT(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "quit") == 0 ||
		strcmp(str, "quit\n") == 0 ? 1 : 0;
}


/////////////////////////
// utils
void token_cleaner(char *token, const char *delim){
	while(token != NULL)
	{
		token = strtok(NULL, delim);
	}
	free(token);
}


char *remove_newline(char *token){
	char *new_token = (char *) malloc( ( strlen(token) - 1 ) );
	memcpy( new_token, token,  ( strlen(token) - 1 ));
	return new_token;
}
