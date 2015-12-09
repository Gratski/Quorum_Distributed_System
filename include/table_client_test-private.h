/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _TABLE_CLIENT_PRIVATE_H
#define _TABLE_CLIENT_PRIVATE_H

#include "client_stub-private.h"


/*
 * Verifica se a string eh PUT
 */
int isPUT(char *str);

/*
 * Verifica se a string eh GET
 */
int isGET(char *str);

/*
 * Verifica se a string eh DEL
 */
int isDEL(char *str);

/*
 * Verifica se a string eh UPDATE
 */
int isUPDATE(char *str);

/*
 * Verifica se a string eh SIZE
 */
int isSIZE(char *size);

/*
 * Verifica se a string eh QUIT
 */
int isTimestamp(char *str);

int isNumOps(char *str);

/*
 * Verifica se a string eh QUIT
 */
int isQUIT(char *quit);

/*
 * Token cleaner
 */
void token_cleaner(char *token, const char *delim);

/*
 * remove newline
 */
char *remove_newline(char *token);

/*
 * Executa put
*/
void exec_put(struct rtable_t *rtable, char *token, const char *delim);

/*
 * Executa get
*/
void exec_get(struct rtable_t *qtable, char *token, const char *delim);

/*
 * Executa del
*/
void exec_del(struct rtable_t *qtable, char *token, const char *delim);

/*
 * Executa update
*/
void exec_update(struct rtable_t *qtable, char *token, const char *delim);

void exec_timestamp(struct rtable_t *rtable, char *token, const char *delim);

void exec_numops(struct rtable_t *rtable);


/*
 * Executa size
*/
void exec_size(struct rtable_t *qtable);


/*
 * Obter lista de hosts a partir de argv
*/
const char **get_hosts(char **str, int num_hosts);

/*
 * Libertar espaco obtido por get_hosts
*/
void free_hosts(const char **hosts);

#endif