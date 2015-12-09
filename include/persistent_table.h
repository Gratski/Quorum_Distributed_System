#ifndef _PERSISTENT_TABLE_H
#define _PERSISTENT_TABLE_H

#include "data.h"
#include "table-private.h"
#include "persistence_manager-private.h"

struct ptable_t; /* A definir em persistent_table-private.h */

/* Abre o acesso a uma tabela persistente, passando como parâmetros a
 * tabela a ser mantida em memória e o gestor de persistência a ser usado
 * para manter logs e checkpoints. Retorna a tabela persistente criada ou
 * NULL em caso de erro.
 */
struct ptable_t *ptable_open(struct table_t *table, 
                             struct pmanager_t *pmanager);

/* Fecha o acesso a uma tabela persistente. Todas as operações em table
 * devem falhar após um ptable_close.
 */
void ptable_close(struct ptable_t *ptable);

/* Liberta toda a memória e apaga todos os ficheiros utilizados pela
 * tabela persistente.
 */
void ptable_destroy(struct ptable_t *ptable);

/* Função para adicionar um par chave valor na tabela.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int ptable_put(struct ptable_t *ptable, char *key, struct data_t *value);

/* Função para substituir na tabela, o valor associado à chave key.
 * Devolve 0 (OK) ou -1 em caso de erros.
 */
int pable_update(struct ptable_t *ptable, char *key, struct data_t *value);

/* Função para obter da tabela o valor associado à chave key.
 * Devolve NULL em caso de erro.
 */
struct data_t *ptable_get(struct ptable_t *ptable, char *key);

/* Função para remover um par chave valor da tabela, especificado pela
 * chave key.
 * Devolve: 0 (OK) ou -1 em caso de erros
 */
int ptable_del(struct ptable_t *ptable, char *key);

/* Devolve número de elementos na tabela.
 */
int ptable_size(struct ptable_t *ptable);

/* Devolve um array de char * com a cópia de todas as keys da tabela
 * e um último elemento a NULL.
 */
char **ptable_get_keys(struct ptable_t *ptable);

/* Liberta a memória alocada por ptable_get_keys().
 */
void ptable_free_keys(char **keys);

#endif
