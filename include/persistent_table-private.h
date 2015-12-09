/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _PERSISTENT_TABLE_PRIVATE_H
#define _PERSISTENT_TABLE_PRIVATE_H

#include "persistent_table.h"

struct ptable_t {
	struct table_t *table;
	struct pmanager_t *pmanager;
	int is_closed;
};

/* Funcao auxiliar que trata de criar um novo ficheiro de logo e o
 * respectivo checkpoint
 * Retorna 0 se tudo estiver OK ou -1 em caso de erro.
 */
int clear_log(struct pmanager_t *pmanager, struct table_t *table);

/* Função para obter o timestamp do valor associado a uma chave.
* Em caso de erro devolve -1. Em caso de chave não encontrada
* devolve 0.
*/
long ptable_get_ts(struct ptable_t *ptable, char *key);

#endif
