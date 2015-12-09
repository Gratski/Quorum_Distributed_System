/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "message-private.h"
#include "persistent_table-private.h"
#include "persistence_manager-private.h"

/* Abre o acesso a uma tabela persistente, passando como parâmetros a
 * tabela a ser mantida em memória e o gestor de persistência a ser usado
 * para manter logs e checkpoints. Retorna a tabela persistente criada ou
 * NULL em caso de erro.
 */
struct ptable_t *ptable_open(struct table_t *table, struct pmanager_t *pmanager) {

	if (table == NULL || pmanager == NULL)
		return NULL;

	struct ptable_t *ptable = (struct ptable_t*) malloc(sizeof(struct ptable_t));

	if (ptable == NULL)
		return NULL;

	ptable->table     = table;
	ptable->pmanager  = pmanager;
	ptable->is_closed = 0;

	return ptable;
}


/* Fecha o acesso a uma tabela persistente. Todas as operações em table
 * devem falhar após um ptable_close.
 */
void ptable_close(struct ptable_t *ptable) {

	if (ptable == NULL)
		return;

	fclose(ptable->pmanager->fd);

	ptable->pmanager->fd = NULL;
	ptable->is_closed    = 1;

	return;
}


/* Liberta toda a memória e apaga todos os ficheiros utilizados pela
 * tabela persistente.
 */
void ptable_destroy(struct ptable_t *ptable) {

	if (ptable == NULL)
		return;

	pmanager_destroy_clear(ptable->pmanager);
	pmanager_destroy(ptable->pmanager);
	free(ptable);

	return;
}


/* Função para adicionar um par chave valor na tabela.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int ptable_put(struct ptable_t *ptable, char *key, struct data_t *value) {

	int res = 0;

	if (ptable == NULL || ptable->is_closed || key == NULL || value == NULL)
		return -1;

	//preparar message para ser convertida
	struct message_t *msg =
		(struct message_t*) malloc(sizeof(struct message_t));

	if (msg == NULL)
		return -1;

	msg->opcode        = OC_PUT;
	msg->c_type        = CT_ENTRY;
	msg->content.entry = entry_create(key, value);

	//erro ao criar entry
	if (msg->content.entry == NULL) {
		free(msg);
		return -1;
	}

	int log_res = 0;
	char* buffer = NULL;
	message_to_buffer(msg, &buffer);

	//persistir instrucao no ficheiro no formato put <key> <value>
	if (table_put(ptable->table, key, value) == 0
		&& (log_res = pmanager_log(ptable->pmanager, buffer) > 0)) {

		ptable->table->num_change_ops++;
	}
	//log cheio
	else if ((log_res == -1)
		&& (clear_log(ptable->pmanager, ptable->table) == 0)) {

		puts("Checkpoint criado.");
		//completar registo no novo log
		res = pmanager_log(ptable->pmanager, buffer);

	} else {
		puts("Erro ao criar checkpoint!");
		res = -1;
	}

	free(msg);
	free(buffer);

	return res;
}


/* Função para substituir na tabela, o valor associado à chave key.
 * Devolve 0 (OK) ou -1 em caso de erros.
 */
int ptable_update(struct ptable_t *ptable, char *key, struct data_t *value) {

	int res = 0;

	if (ptable == NULL || ptable->is_closed || key == NULL || value == NULL)
		return -1;

	//preparar message para ser convertida
	struct message_t *msg = malloc(sizeof(struct message_t));

	if (msg == NULL)
		return -1;

	msg->opcode        = OC_UPDATE;
	msg->c_type        = CT_ENTRY;
	msg->content.entry = entry_create(key, value);

	//erro ao criar entry
	if (msg->content.entry == NULL) {
		free(msg);
		return -1;
	}

	int log_res = 0;
	char* buffer = NULL;
	message_to_buffer(msg, &buffer);

	//persiste instrucao no ficheiro
	if (table_update(ptable->table, key, value) == 0
		&& (log_res = pmanager_log(ptable->pmanager, buffer) > 0)) {

		ptable->table->num_change_ops++;
	}
	//log cheio?
	else if ((log_res == -1)
		&& (clear_log(ptable->pmanager, ptable->table) == 0)) {

		puts("Checkpoint criado.");
		//completar registo no novo log
		res = pmanager_log(ptable->pmanager, buffer);

	} else {
		puts("Erro ao criar checkpoint!");
		res = -1;
	}

	return res;
}


/* Função para obter da tabela o valor associado à chave key.
 * Devolve NULL em caso de erro.
 */
struct data_t *ptable_get(struct ptable_t *ptable, char *key) {

	if (ptable == NULL || ptable->is_closed || key == NULL)
		return NULL;

	return table_get(ptable->table, key);
}


/* Função para remover um par chave valor da tabela, especificado pela
 * chave key.
 * Devolve: 0 (OK) ou -1 em caso de erros
 */
int ptable_del(struct ptable_t *ptable, char *key) {

	int res = 0;

	if (ptable == NULL || key == NULL || ptable->is_closed)
		return -1;

	int log_res = 0;

	if(table_del(ptable->table, key) == 0
		&& (log_res = pmanager_log(ptable->pmanager, key) > 0)) {

		ptable->table->num_change_ops++;
	}
	//log cheio?
	else if (log_res == -1
		&& (clear_log(ptable->pmanager, ptable->table) == 0)) {

		puts("Checkpoint criado.");
		//completar registo no novo log
		res = pmanager_log(ptable->pmanager, key);

	} else {
		puts("Erro ao criar checkpoint!");
		res = -1;
	}

	return res;
}


/* Devolve número de elementos na tabela.
 */
int ptable_size(struct ptable_t *ptable) {

	if (ptable == NULL || ptable->is_closed)
		return -1;

	return table_size(ptable->table);
}


/* Devolve um array de char * com a cópia de todas as keys da tabela
 * e um último elemento a NULL.
 */
char **ptable_get_keys(struct ptable_t *ptable) {

	if (ptable == NULL || ptable->is_closed)
		return NULL;

	return table_get_keys(ptable->table);
}


/* Liberta a memória alocada por ptable_get_keys().
 */
void ptable_free_keys(char **keys) {

	table_free_keys(keys);

	return;
}


int clear_log(struct pmanager_t *pmanager, struct table_t *table) {

	if ((pmanager_store_table(pmanager, table) == 0)
		&& (pmanager_rotate_log(pmanager) == 0))
		return 0;
	else
		return -1;

}


long ptable_get_ts(struct ptable_t *ptable, char *key) {

	if (ptable == NULL || key == NULL)
		return -1;

	struct data_t *data = ptable_get(ptable, key);

	if (data == NULL)
		return 0;

	return data->timestamp;

}


