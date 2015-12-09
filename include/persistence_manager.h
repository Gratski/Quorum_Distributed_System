#ifndef _PERSISTENCE_MANAGER_H
#define _PERSISTENCE_MANAGER_H

#include "table-private.h"

struct pmanager_t; /* A definir em persistence_manager-private.h */

/* Cria um gestor de persistência que armazena logs em filename+".log".
 * O parâmetro logsize define o tamanho máximo em bytes que o ficheiro de
 * log pode ter.
 * Note que filename pode ser um path completo. Retorna o pmanager criado
 * ou NULL em caso de erro.
 */
struct pmanager_t *pmanager_create(char *filename, int logsize);

/* Destrói o gestor de persistência pmanager. Retorna 0 se tudo estiver OK
 * ou -1 em caso de erro. Esta função não limpa o ficheiro de log.
 */
int pmanager_destroy(struct pmanager_t *pmanager);

/* Apaga o ficheiro de log gerido pelo gestor de persistência.
 * Retorna 0 se tudo estiver OK ou -1 em caso de erro.
 */
int pmanager_destroy_clear(struct pmanager_t *pmanager);

/* Retorna 1 caso existam dados no ficheiro de log e 0 caso contrário.
 */
int pmanager_has_data(struct pmanager_t *pmanager);

/* Adiciona uma string msg no fim do ficheiro de log associado a pmanager.
 * Retorna o numero de bytes escritos no log ou -1 em caso de problemas na
 * escrita (e.g., erro no write()), ou no caso em que o tamanho do ficheiro
 * de log após o armazenamento da mensagem seja maior que logsize (neste
 * caso msg não é escrita no log).
 */
int pmanager_log(struct pmanager_t *pmanager, char *msg);

/* Recupera o estado contido no ficheiro de log, na tabela passada como
 * argumento.
 */
int pmanager_fill_state(struct pmanager_t *pmanager,
                        struct table_t *table);

#endif
