/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#define MAX_TRIES 1
#define OC_RT_GETTS 60

#include "client_stub.h"

struct rtable_t{
	struct server_t *server;
};


/* Função para obter o timestamp do valor associado a essa chave.
* Em caso de erro devolve -1. Em caso de chave não encontrada
* devolve 0.
*/
long long rtable_get_ts(struct rtable_t *rtable, char *key);


int rtable_num_ops(struct rtable_t *rtable);

#endif

