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
#include "network_client.h"
#include "network_client-private.h"
#include "table_client-private.h"
#include "client_stub-private.h"
#include "quorum_table-private.h"

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
		puts("Exemplo de uso: ./table_client <id_client> <servidor0>:<porto0> ... <servidorN>:<portoN>");
		return -1;
	}

	//ignorar sinais do tipo SIGPIPE
   if (ignsigpipe() != 0){
      perror("ignsigpipe falhou");
      exit(1);
   }

	char *cmd  = NULL;
	size_t max = MAX_MSG + 1;
	const char delim[2] = " ";

	//estabelece ligacao
	struct qtable_t *qtable = (struct qtable_t *) qtable_bind((const) argv, (argc - 2));
	if( qtable == NULL )
	{
		puts("Nao existem condicoes para inicializar o quorum");
		return -1;
	}
	else{
		puts("Ligações estabelecidas");
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
		puts("quit\n");
		puts("Inserir comando: ");

		// obtem input

		getline(&cmd, &max, stdin);
		char *token = strtok(cmd, delim);

		// PUT
		if( isPUT(token) )
		{
			exec_put(qtable, token, delim);
		}
		// GET
		else if( isGET(token) )
		{
			exec_get(qtable, token, delim);
		}
		// DEL
		else if( isDEL(token) )
		{
			exec_del(qtable, token, delim);
		}
		// UPDATE
		else if( isUPDATE(token) )
		{
			exec_update(qtable, token, delim);
		}
		
		// SIZE
		else if( isSIZE(token) )
		{
			exec_size(qtable);
		}
		// QUIT
		else if( isQUIT( token ) )
		{
			break;
		}
		else
			puts("\nComando invalido!");

		// clean token
		token_cleaner(token, delim);
	} while( !isQUIT(cmd) && qtable != NULL);

	//desconectar dos servidores
	qtable_disconnect(qtable);
	free(cmd);



	return 0;
}


// exec methods
void exec_put(struct qtable_t *qtable, char *token, const char *delim){
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
		struct data_t *data = data_create2(strlen(value) + 1, value);

		//passa ah api
		int res = qtable_put(qtable, key, data);
		if( res == -2 )
			puts("Impossivel estabelecer meio termo");
		else if( res == -1 )
			puts("Entry ja existe");
		else
			puts("Entry inserida com sucesso");

		free(value);
		data_destroy(data);
	}

	free(key);
}


void exec_get(struct qtable_t *qtable, char *token, const char *delim){
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
		char **keys = qtable_get_keys(qtable);

		if( keys != NULL )
		{
			int i = 0;

			for(i = 0; keys[i] != NULL; i++)
			{
				printf("Key %d: %s\n", i + 1, keys[i]);
			}

			//rtable_free_keys(keys);
		}else
			puts("Erro ao obter keys");
	}
	// se eh apenas uma key
	else
	{
		struct data_t *data = qtable_get(qtable, token);

		// se obteve resposta
		if( data == NULL )
		{
			puts("Impossivel estabelecer meio termo");
		}
		else if( data != NULL && data->data != NULL && data->datasize > -1 )
		{
			printf("Key: %s | Value: %s\n", token, (char*) data->data);
			data_destroy(data);
		}
		// se nao obteve resposta
		else{
			printf("Entry inexistente.\n");
			//notifica que terminou ligacao
		}
	}
}


void exec_del(struct qtable_t *qtable, char *token, const char *delim){
	const char valor[2] = "\n";
	token = strtok(NULL, valor);

	if( token == NULL ){
		puts("\nComando del invalido!");
		puts("Exemplo de uso: del <key>\n");
		return;
	}

	int result_code = qtable_del(qtable, token);
	puts("");
	//se nao recebeu resposta
	if( result_code == -2 )
		puts("Impossivel estabelecer meio termo");
	else if( result_code == -1 ){
		//se server fechou connection
		//noptifica
		puts("Entry inexistente...");
	}
	else{
		//se foi eliminada
		puts("Entry eliminada!");
	}
}

void exec_update(struct qtable_t *qtable, char *token, const char *delim){
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
	struct data_t *data = data_create2(strlen(value) + 1, value);

	// faz pedido
	int result_code = qtable_update(qtable, key, data);

	//se nao recebeu resposta
	if( result_code == -2 )
	{
		puts("Impossivel estabelecer meio termo");
	}
	else if( result_code == -1 )
	{
		puts("Entry inexistente");
	}
	else
	{
		puts("Entry actualizada!");
	}

	//libertacao de memoria
	free(key);
	free(value);
	data_destroy(data);
}

void exec_size(struct qtable_t *qtable){
	if ( qtable == NULL)
		return;

	int size = qtable_size(qtable);

	//se nao recebeu resposta
	if ( size < 0 )
	{
		puts("Erro ao obter size...");
	} else {
		//se recebeu bem o size
		printf("Size: %d\n", size);

	}
}


/////////////////////////
// verifiers
int isPUT(char *str){
	if (str == NULL)
		return 0;

	return
		strcmp(str, "put") == 0 ||
		strcmp(str, "put\n") == 0 ? 1 : 0;
}


int isGET(char *str){
	if (str == NULL)
		return 0;

	return
		strcmp(str, "get") == 0 ||
		strcmp(str, "get\n") == 0 ? 1 : 0;
}


int isDEL(char *str){
	if (str == NULL)
		return 0;

	return
		strcmp(str, "del") == 0 ||
		strcmp(str, "del\n") == 0 ? 1 : 0;
}


int isUPDATE(char *str){
	if (str == NULL)
		return 0;

	return
		strcmp(str, "update") == 0 ||
		strcmp(str, "update\n") == 0 ? 1 : 0;
}


int isSIZE(char *str){
	if (str == NULL)
		return 0;

	return
		strcmp(str, "size") == 0 ||
		strcmp(str, "size\n") == 0 ? 1 : 0;
}


int isQUIT(char *str){
	if (str == NULL)
		return 0;

	return
		strcmp(str, "quit") == 0 ||
		strcmp(str, "quit\n") == 0 ? 1 : 0;
}


int isTimestamp(char *str){
	if(str == NULL)
		return 0;

	return
		strcmp(str, "timestamp") == 0 ||
		strcmp(str, "timestamp\n") == 0 ? 1 : 0;
}


/////////////////////////
// utils
void token_cleaner(char *token, const char *delim){
	while (token != NULL)
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


const char **get_hosts(char **str, int num_hosts) {
	const char** hosts = (const char **) malloc((sizeof(char*) * (num_hosts + 1)));

	int i;
	for (i = 0; i < num_hosts; i++) {
		hosts[i] = strdup(str[i]);
	}

	//terminar array
	hosts[i] = NULL;

	return hosts;
}


void free_hosts(const char **hosts) {

	int i;

	for (i = 0; hosts[i] != NULL; i++) {
		free((char*) hosts[i]);
	}

	free(hosts);
}
