/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _DATA_PRIVATE_H
#define _DATA_PRIVATE_H

#include "data.h"

/* Funcao que permite criar um tipo data recorrendo a fucano data_create2
 * Devolve um tipo data ou NULL em caso de erro
 */
struct data_t *data_create3(int size, void * data, long long timestamp);

/* Funcao que incrementa o valor de um timestamp.
 * Devolve o valor do timestamp atualizado ou -1 em caso de erro.
 */
long long inc_timestamp(long long timestamp, int id);

#endif
