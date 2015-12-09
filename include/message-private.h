/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

//dimensoes dos tipos de dados
#define SHORT_SIZE	2
#define INT_SIZE  	4
#define LONG_SIZE 	16

#define CT_TIMESTAMP 60
#define OC_NUM_OPS   70
#define OC_RT_ERROR  99

#include "message.h"
#include "entry.h"
#include "data.h"

/* Funcao que permite validar o opcode
 * de uma determinada mensagem
 * Devolve: 0 (OK), -1 (opcode invalido)
 */
int validate_opcode(int opcode);

/* Funcao que permite validar o c_type
 * de uma determinada mensagem
 * Devolve: 0 (OK), -1 (opcode invalido)
 */
int validate_c_type(int c_type);

/* Funcao que permite validar o c_type
 * de uma determinada mensagem
 * Devolve: 0 (OK), -1 (c_type invalido)
 */
int validate_data(struct data_t *data);

/* Funcao que permite validar uma determinada entry
 * Devolve: 0 (OK), -1 (entry invalida)
 */
int validate_entry(struct entry_t *entry);

/* Funcao que permite validar uma determinada message
 * Devolve: 0 (OK), -1 (message invalida)
 */
int validate_msg(struct message_t *msg);

/* Funcao que escreve o conteudo de um buffer
 * atraves do socket em param
 * Devolve: numero de bytes escritos ou -1 (erro)
 */
int write_all(int socket, char *buf, int buf_size);

/* Funcao que le o conteudo através de um socket em params
 * para o buffer "buf" em param
 * Devolve: numero de bytes escritos ou -1 (erro)
 */
int read_all(int socket, char **buf, int buf_size);

/* Funcao que le o conteudo através de um socket em params
 * para o buffer "buf" em param
 * Devolve: numero de bytes escritos ou -1 (erro)
 */
long long swap_bytes_64(long long number);

#endif

