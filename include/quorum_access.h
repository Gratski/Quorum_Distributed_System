#ifndef _QUORUM_ACCESS_H
#define _QUORUM_ACCESS_H

#include "client_stub-private.h"
#include "message.h"

/* Estrutura que agrega a informação acerca da operação a ser executada
 * pelas threads através da tabela remota implementada no módulo
 * client_stub.
 */
struct quorum_op_t {
	int id; /* id único da operação (o mesmo no pedido e na resposta) */
	int sender; /* emissor da operação, ou da resposta */
	int opcode; /* o mesmo usado em message_t.opcode */
	union content_u content;
};

/* Esta função deve criar as threads e as filas de comunicação para que o
 * cliente invoque operações a um conjunto de tabelas em servidores
 * remotos. Recebe como parâmetro um array rtable de tamanho n.
 * Retorna 0 (OK) ou -1 (erro).
 */
int init_quorum_access(struct rtable_t *rtable, int n);

/* Função que envia um pedido a um conjunto de servidores e devolve
 * um array com o número esperado de respostas.
 * O parâmetro request é uma representação do pedido, enquanto
 * expected_replies representa a quantidade de respostas esperadas antes 
 * da função retornar.
 * Note que os campos id e sender em request serão preenchidos dentro da
 * função. O array retornado é um array com k posições (0 a k-1), sendo cada
 * posição correspondente a um apontador para a resposta de um servidor 
 * pertencente ao quórum que foi contactado.
 * Caso não se consigam respostas do quórum mínimo, deve-se retornar NULL.
 */
struct quorum_op_t **quorum_access(struct quorum_op_t *request, 
                                   int expected_replies);

/* Liberta a memoria, destroi as rtables usadas e destroi as threads.
 */
int destroy_quorum_access();

#endif
