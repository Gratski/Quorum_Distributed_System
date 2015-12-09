#ifndef _QUORUM_TABLE_H
#define _QUORUM_TABLE_H

#include "data.h"

/* Define os possíveis opcodes da mensagem */
#define OC_SIZE	10
#define OC_DEL		20
#define OC_UPDATE 30
#define OC_GET		40
#define OC_PUT		50

/* A definir pelo grupo em quorum_table-private.h */
struct qtable_t;

/* Função para estabelecer uma associação entre uma tabela qtable_t e
 * um array de n servidores.
 * addresses_ports é um array de strings e n é o tamanho deste array.
 * Retorna NULL caso não consiga criar o qtable_t.
 */
struct qtable_t *qtable_bind(const char **addresses_ports, int n);

/* Fecha a ligação com os servidores do sistema e liberta a memória alocada
 * para qtable. 
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int qtable_disconnect(struct qtable_t *qtable);

/* Função para adicionar um elemento na tabela.
 * Note que o timestamp em value será atribuído internamente a esta função,
 * como definido no algoritmo de escrita.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int qtable_put(struct qtable_t *qtable, char *key, struct data_t *value);

/* Função para atualizar o valor associado a uma chave.
 * Note que o timestamp em value será atribuído internamente a esta função,
 * como definido no algoritmo de update.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int qtable_update(struct qtable_t *qtable, char *key,
                  struct data_t *value);

/* Função para obter um elemento da tabela.
 * Em caso de erro ou elemento não existente, devolve NULL.
 */
struct data_t *qtable_get(struct qtable_t *qtable, char *key);

/* Função para remover um elemento da tabela. É equivalente à execução 
 * put(k, NULL) se a chave existir. Se a chave não existir, nada acontece.
 * Devolve 0 (ok), -1 (chave não encontrada).
 */
int qtable_del(struct qtable_t *qtable, char *key);

/* Devolve número (aproximado) de elementos da tabela ou -1 em caso de
 * erro.
 */
int qtable_size(struct qtable_t *qtable);

/* Devolve um array de char* com a cópia de todas as keys da tabela,
 * e um último elemento a NULL. Esta função não deve retornar as
 * chaves removidas, i.e., a NULL.
 */
char **qtable_get_keys(struct qtable_t *qtable);

/* Liberta a memória alocada por qtable_get_keys().
 */
void qtable_free_keys(char **keys);

#endif
