/* 	-- Grupo 3 --
  	João Gouveia 	nº 45679
 	João Rodrigues	nº 45582
   Pedro Luís 		nº 45588
*/

#ifndef _PERSISTENCE_MANAGER_PRIVATE_H
#define _PERSISTENCE_MANAGER_PRIVATE_H

#include "persistence_manager.h"

struct pmanager_t {
	char* file_name;
	int   file_size;
	int   max_file_size;
	FILE  *fd;	//file descriptor
};


/* Determina se um determinado ficheiro foi completamente escrito.
 * Devolve 1 caso tenha sido completamente escrito, 0 caso contrario e -1 em
 * caso de erro.
 */
int is_complete(char* file_name);

/* Determina se um determinado ficheiro se encontra vazio.
 * Devolve 1 caso o ficheiro nao tenha conteudo, 0 caso tenha conteudo
 * e -1 em caso de erro.
 */
int is_file_empty(char* file_name);

/* Obtem o conteudo de um determinado ficheiro de log para a tabela table.
 * Devolve o numero de bytes lidos do ficheiro ou -1 em caso de erro.
 */
int load_file(char* file_type, struct table_t *table);

/* Obtem o conteudo do ficheiro de log para a tabela table.
 * Devolve o numero de bytes lidos do ficheiro ou -1 em caso de erro.
 */
int load_log_file(char* file_type, struct table_t *table);

/*
 * Obtem o tamanho de um ficheiro de nome file_name.
 * Devolve o tamanho do ficheiro ou -1 em caso de erro.
 */
int file_size(char *file_name);

/*
 * Concatena uma determinada extensao file_type ao nome de ficheiro file_name.
 * Devolve o nome do ficheiro concatenado com a respectiva extensao ou NULL
 * em caso de erro.
 */
char *append_file_type(char *file_name, char *file_type);

/* Cria um ficheiro filename+".stt" com o estado atual da tabela table.
 * Devolve o tamanho do ficheiro criado ou -1 em caso de erro.
 */
int pmanager_store_table(struct pmanager_t *pmanager, struct table_t *table);

/* Limpa o conteúdo do ficheiro ".log" e copia o conteudo do ficheiro ".stt"
 * para o ficheiro ".ckp".
 * Devolve 0 se tudo correr bem ou -1 em caso de erro.
 */
int pmanager_rotate_log(struct pmanager_t *pmanager);

#endif
