/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _TABLE_CLIENT_PRIVATE_H
#define _TABLE_CLIENT_PRIVATE_H

#include "client_stub-private.h"
#include "quorum_table-private.h"


/* Verifica se a string eh PUT
 *
 */
int isPUT(char *str);

/* Verifica se a string eh GET
 *
 */
int isGET(char *str);

/* Verifica se a string eh DEL
 *
 */
int isDEL(char *str);

/* Verifica se a string eh UPDATE
 *
 */
int isUPDATE(char *str);

/*
 * Verifica se a string eh SIZE
 */
int isSIZE(char *size);

/* Verifica se a string eh TIMESIZE
 *
 */
int isTimestamp(char *str);

/* Verifica se a string eh QUIT
 *
 */
int isQUIT(char *quit);

/* Token cleaner
 *
 */
void token_cleaner(char *token, const char *delim);

/* Remove newline de um token
 *
 */
char *remove_newline(char *token);

/* Executa um pedido de PUT na qtable
 *
*/
void exec_put(struct qtable_t *qtable, char *token, const char *delim);

/* Executa um pedido de GET na qtable
 *
*/
void exec_get(struct qtable_t *qtable, char *token, const char *delim);

/* Executa um pedido de DEL na qtable
 *
*/
void exec_del(struct qtable_t *qtable, char *token, const char *delim);

/* Executa um pedido de UPDATE na qtable
 *
*/
void exec_update(struct qtable_t *qtable, char *token, const char *delim);

/* Executa um pedido de SIZE na qtable
 *
*/
void exec_size(struct qtable_t *qtable);

/* Executa um pedido de TIMESTAMP na qtable
 *
*/
void exec_timestamp(struct qtable_t *qtable, char *token, const char *delim);

/* Obtem uma lista de hosts a partir de argv
 *
*/
const char **get_hosts(char **str, int num_hosts);

/* Liberta a memoria obtida por get_hosts
 *
*/
void free_hosts(const char **hosts);

#endif