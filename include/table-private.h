/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H

#include "table.h"

struct table_t{
	int size;
	int num_places;
	int num_change_ops;
	struct list_t **places;
};

/* Funcao que permite obter o hashcode
 * de uma determinada key
 * Devolve: 0 <= hashcode < numero de buckets (OK) -1 (em caso de erro)
 */
int hashcode(struct table_t *table, char *key);

/* Retorna o número de alterações realizadas na tabela.
*/
int table_get_num_change_ops(struct table_t *table);

/* Devolve um array de entry_t* com cópias de todas as entries
* da tabela, e um último elemento a NULL.
*/
struct entry_t **table_get_entries(struct table_t *table);

/* Liberta a memória alocada por table_get_entries().
*/
void table_free_entries(struct entry_t **entries);

/* Função para obter o timestamp do valor associado a uma chave.
* Em caso de erro devolve -1. Em caso de chave não encontrada devolve 0.
*/
long long table_get_ts(struct table_t *table, char *key);

#endif