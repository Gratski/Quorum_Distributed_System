/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#include "inet.h"
#include "network_client.h"

#define RETRY_TIME 5

//estrutura descritora de socket
struct server_t{
	int socket;
	int port;
	char *hostname;
	int is_connected; // -1 not connected, 1 connected
	struct sockaddr_in specs;
};

/* Abre socket com base nos dados de struct server_t
 */
void socket_open(struct server_t *server);

/* Auxiliar para enviar e receber mensagem.
 * Devolve uma mensagem de resposta ou NULL em caso de erro.
 */
struct message_t *send_receive_aux( struct server_t *server, int bytes, char *buffer );

int floor(double n);
