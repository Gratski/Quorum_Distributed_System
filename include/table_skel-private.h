/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _TABLE_SKEL_PRIVATE_H
#define _TABLE_SKEL_PRIVATE_H

#include "message-private.h"
#include "persistent_table-private.h"

#define NUMFD   10
#define TIMEOUT 50

/*
 * Metodos reescritos
 */
int table_skel_init(int n_lists, char *filename, int logsize);

int table_skel_destroy();

/* Executa uma operação (indicada pelo opcode na msg_in) e retorna o
 * resultado numa mensagem de resposta ou NULL em caso de erro.
 */
struct message_t *invoke(struct message_t *msg_in);

/* Determina se o ficheiro de log ira ultrapassar o seu limite com a escrita
 * da mensagem client_msg.
 * Devolve 0 caso o ficheiro de log tenha sido alterado e -1 em caso de erros.
 */
int check_log_size(struct ptable_t *ptable, struct message_t *client_msg);

/* Função que permite receber uma mensagem enviada por um client através
 * do file descriptor de um socket.
 * Devolve a mensagem enviada por um client.
 */
struct message_t *network_receive(int connsockfd);

/* Função que permite enviar uma determinada mensagem a um client através
 * do file descriptor de um socket.
 * Devolve 0 caso tenha sido bem sucessido e -1 caso contrário.
 */
int network_send(int connsockfd, struct message_t *msg);

/* Função que permite converter uma determinada mensagem numa linha a ser
 * introduzida no ficheiro de log
 * Devolve a mensagem a ser escrita ou NULL em caso de erro
 */
char *prepare_log_entry(struct message_t *msg);

/* Função que permite obter uma mensagem de erro.
 * Devolve uma mensagem de erro ou NULL (out of memory, outros erros)
 */
struct message_t *get_error_msg();

/* Função que permite executar uma operacao de PUT numa determinada hash table.
 * Devolve uma mensagem (de confirmacao ou de erro) ou NULL (out of memory,
 * outros erros)
 */
struct message_t *exec_put_msg(struct ptable_t *ptable, struct message_t *client_msg);

/* Função que permite executar uma operacao de GET numa determinada hash table.
 * Devolve uma mensagem (contendo uma struct data relativa ao conteudo da key,
 * ou de erro) ou NULL (out of memory, outros erros)
 */
struct message_t *exec_get_msg(struct table_t *table, struct message_t *client_msg);

/* Função que permite obter uma mensagem contendo todas as keys de uma
 * determinada hash table.
 * Devolve uma mensagem (contendo todas as keys da tabela table ou de erro) ou
 * NULL (out of memory, outros erros)
 */
struct message_t *exec_get_keys_msg(struct table_t *table);

/* Função que permite executar uma operacao de DELETE numa determinada hash
 * table.
 * Devolve uma mensagem de confirmacao ou NULL (out of memory, outros erros)
 */
struct message_t *exec_del_msg(struct table_t *table, struct message_t *client_msg);

/* Função que permite executar uma operacao de UPDATE numa determinada hash
 * table.
 * Devolve uma mensagem de confirmacao ou NULL (out of memory, outros erros)
 */
struct message_t *exec_update_msg(struct table_t *table, struct message_t *client_msg);

/* Função que permite obter o numero de keys presentes numa determinada hash
 * table.
 * Devolve uma mensagem de resposta contendo a dimensao da hash table ou NULL
 * (out of memory, outros erros).
 */
struct message_t *exec_get_size_msg(struct table_t *table);

/* Função que permite obter o numero de operacoes realizadas numa determinada table.
 * Devolve uma mensagem contendo o numero de operacoes da table ou NULL (out of
 * memory, outros erros).
 */
struct message_t *exec_get_num_change_ops(struct table_t *table);

/* Função que permite obter o valor do timestamp de uma entry da table.
 * Devolve uma mensagem contendo o valor do timestamp ou NULL (out of memory,
 * outros erros).
 */
struct message_t *exec_get_timestamp(struct table_t *table, struct message_t *client_msg);

#endif