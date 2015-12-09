/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#include "string.h"
#include "inet.h"
#include "message.h"
#include "message-private.h"
#include "network_client-private.h"


void socket_open(struct server_t *server){

	//open socket
	if( (server->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		server->is_connected = -1;
		return;
	}

	//point to server
	if( ( inet_pton( AF_INET, server->hostname, &(server->specs.sin_addr) ) ) < 1 )
	{
		server->is_connected = -1;
		return;
	}

	//estabelecer connection
	if( ( connect( server->socket, ( struct sockaddr * ) &(server->specs), sizeof(server->specs) ) ) < 0 )
	{
		server->is_connected = -1;
		return;
	}

	server->is_connected = 1;
}


struct server_t *network_connect(const char *address_port){
	struct server_t *server = (struct server_t *) malloc( sizeof(struct server_t) );
	if( server == NULL)
		return NULL;

	if(address_port == NULL){
		free(server);
		return NULL;
	}

	const char delim[2] = ":";
	char *token = strtok((char *)address_port, delim);

	//atribuir ip de servidor
	server->hostname = strdup(token);

	//atribuir porto de rede
	token = strtok(NULL, delim);
	server->port = atoi(token);

	server->specs.sin_family = AF_INET;
	server->specs.sin_port = server->port;

	//open socket
	socket_open(server);
	if( server->is_connected != 1 )
		return NULL;

	return server;
}


struct message_t *network_send_receive(struct server_t *server, struct message_t *msg){
	if( server == NULL || msg == NULL )
		return NULL;

	// prepara envio de mensagem
	char *buffer = NULL;
	int buffer_size = message_to_buffer( msg, &buffer );

	// envia e recebe de server
	/*
	struct message_t *rsp = send_receive_aux(server, buffer_size, buffer);
	if( rsp == NULL ){
		puts("Falhou comunicacao");
		puts("Retrying...");
		sleep(RETRY_TIME);
		socket_open(server);

		if (server->is_connected == 1) {
			rsp = send_receive_aux(server, buffer_size, buffer);
		}
	}
	*/

	struct message_t *rsp;

	while((rsp = send_receive_aux(server, buffer_size, buffer)) == NULL) {
		server->is_connected = -1;
		sleep(RETRY_TIME);
		socket_open(server);
	}

	server->is_connected = 1;
	if( buffer != NULL )
		free(buffer);

	
	return rsp;
}


int network_close(struct server_t *server){
	if (server != NULL) {
		close(server->socket);
		free(server->hostname);
		free(server);
		return 0;
	}

	return -1;
}


struct message_t *send_receive_aux(struct server_t *server, int buffer_size, char *buffer){

	int op, ct;
	memcpy(&op, buffer, 2);
	op = ntohs(op);
	memcpy(&ct, buffer + 2, 2);
	ct = ntohs(ct);

	int nbytes;
	int buffer_size_net = htonl(buffer_size);

	// ENVIA
	char *str = (char *) malloc( INT_SIZE );
	memcpy( str, &buffer_size_net, INT_SIZE );

	//envia tamanho de mensagem a ler
	nbytes = write_all(server->socket, str, INT_SIZE);

	if ( nbytes < 0 )
	{
		free(str);
		server->is_connected = -1;
		return NULL;
	}

	free(str);

	//envia mensagem
	nbytes = write_all( server->socket, buffer, buffer_size );

	if ( nbytes <= 0 )
	{
		server->is_connected = -1;
		return NULL;
	}

//	puts("Pedido enviado");
//	puts("Aguarda resposta...");

	//le tamanho mensagem
	int rsp_size;
	char *rsp_str = NULL;
	nbytes = read_all( server->socket, &rsp_str, INT_SIZE );

	if ( nbytes <= 0 )
	{
		//puts("Server disconnected!");
		server->is_connected = -1;
		return NULL;
	}

	//le mensagem de tamanho lido
	memcpy(&rsp_size, rsp_str, INT_SIZE);
	rsp_size = ntohl(rsp_size);

	if ( rsp_size < 0 ) {
		//puts("Response size < 0!");
		free(rsp_str);
		return NULL;
	}

	free(rsp_str);

	char *buffer_rsp = NULL;
	nbytes = read_all( server->socket, &buffer_rsp , rsp_size);

	if ( nbytes <= 0 )
	{
		server->is_connected = -1;
		return NULL;

	}

	//obtem mensagem a partir de buffer
	struct message_t *rsp_msg = buffer_to_message(buffer_rsp, rsp_size);

	if (rsp_msg == NULL){
		//printf("Error no message to buffer\n");
		return NULL;
	}

	free(buffer_rsp);

	return rsp_msg;
}


int floor(double n){
	return (int) n;
}
