/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list-private.h"
#include "table-private.h"
#include "data-private.h"

int hashcode(struct table_t *table, char *key){
	if (table == NULL || key == NULL)
		return -1;

	int i, val = 0;
	int key_length = strlen(key);

	//se key tem 6 ou menos letras
	if ( key_length <= 6 )
	{
		for (i = 0; i < key_length; i++)
			val += (int) key[i];
	}
	//se key tem mais que 6 letras
	else
	{
		//soma as 3 primeiras letras
		for (i = 0; i < 3; i++)
			val += (int) key[i];

		//soma as 3 ultimas letras
		for (i = 0; i < 3; i++)
			val += (int) key[key_length - i - 1];
	}

	return val % table->num_places;
}


struct table_t *table_create(int n) {
	if (n <= 0)
		return NULL;

	struct table_t *table = (struct table_t *) malloc(sizeof(struct table_t));

	if (table == NULL)
		return NULL;

	table->size           = 0;
	table->num_change_ops = 0;
	table->num_places     = n;
	table->places = (struct list_t **) malloc(sizeof(struct list_t *) * n);

	if (table->places == NULL) {
		free(table);
		return NULL;
	}

	int i;
	// inicializar buckets a NULL
	for (i = 0; i < n; i++)
		table->places[i] = NULL;

	return table;
}


void table_destroy(struct table_t *table){
	if (table == NULL)
		return;

	int i;

	for (i = 0; i < table->num_places; i++)
		list_destroy(table->places[i]);

	free(table->places);
	free(table);
}


int table_put(struct table_t *table, char *key, struct data_t *value){
	int res = -1;

	if (table == NULL || key == NULL || value == NULL)
		return res;

	int pos = hashcode(table, key);
	struct entry_t *entry = list_get(table->places[pos], key);

	//entry ja existe e esta marcada como eliminada
	//atualizar entry para o novo valor
	if (entry != NULL && entry->value->data == NULL) {
		data_destroy(entry->value);
		entry->value = data_dup(value);
		table->size++;
	
		return 0;
	}
	//entry nao existe
	else if (entry == NULL)
	{
		entry = entry_create(key, value);
		int pos = hashcode(table, key);

		// bucket sem lista
		if (table->places[pos] == NULL)
			table->places[pos] = list_create();

		res = list_add(table->places[pos], entry);

		if(res == 0) {
			table->size++;
			table->num_change_ops++;
		}

		entry_destroy(entry);
		res = 0;
	}

	return res;
}


int table_update(struct table_t *table, char *key, struct data_t *value){
	if (table == NULL || key == NULL)
		return -1;

	int pos = hashcode(table, key);

	struct entry_t *entry = list_get(table->places[pos], key);
	
	/* nao atualizar */
	//entry nao existe OR
	//existe, ja esta marcada como eliminada e estamos a tentar eliminar novamente OR
	//existe e tem um valor de timestamp superior
	if( entry == NULL || entry->value->data == NULL ||
		(value->timestamp <= entry->value->timestamp)) {
		return -1;
	}
	//marcar entry como eliminada
	else if (value->data == NULL)
	{
		entry->value->data      = NULL;
		entry->value->datasize  = 0;
		entry->value->timestamp = value->timestamp;
		table->size--;
	}
	//atualizar entry
	else
	{
		data_destroy(entry->value);
		entry->value = data_dup(value);
	}

	table->num_change_ops++;

	return 0;
}


struct data_t *table_get(struct table_t *table, char *key){
	if (table == NULL || key == NULL)
		return NULL;

	int pos = hashcode(table, key);

	if (table->places[pos] == NULL)
		return NULL;

	struct entry_t *entry = list_get(table->places[pos], key);

	if (entry == NULL) {
		puts("Entry nao existe!");
		return NULL;
	}

	return data_dup(entry->value);
}


int table_del(struct table_t *table, char *key){
	if (table == NULL || key == NULL)
		return -1;

	int pos = hashcode(table, key);

	//int res = list_remove(table->places[pos], key);
	int res = table_update(table, key, NULL);

	// se removeu
	if(res == 0)
	{
		table->size--;
		table->num_change_ops++;

		// se a lista em place ficou vazia
		if(table->places[pos]->size == 0)
			table->places[pos] = NULL;
	}

	return res;
}


int table_size(struct table_t *table){
	if (table == NULL)
		return -1;

	return table->size;
}


char **table_get_keys(struct table_t *table){
	if (table == NULL)
		return NULL;

	int i = 0;
	int num_keys = 0;
	char **keys = (char **) malloc(sizeof(char *) * (table_size(table) + 1));

	// percorrer cada bucket da table
	for(i = 0; i < table->num_places; i++){
		// bucket foi inicializado
		if(table->places[i] != NULL){
			char **l = list_get_keys(table->places[i]);

			// bucket nao inicializado
			if (l == NULL)
				return NULL;

			int j;
			// percorrer cada key da lista
			for(j = 0; l[j] != NULL; j++){
				keys[num_keys] = strdup(l[j]);
				num_keys++;
			}

			//list_free_keys(l);
		}
	}

	// terminar array de keys
	keys[table_size(table)] = NULL;

	return keys;
}


void table_free_keys(char **keys){
	if (keys == NULL)
		return;

	int i;
	for(i = 0; keys[i] != NULL; i++)
		free(keys[i]);

	free(keys);
}


/* Retorna o número de alterações realizadas na tabela.
*/
int table_get_num_change_ops(struct table_t *table) {
	return table->num_change_ops;
}


/* Devolve um array de entry_t* com cópias de todas as entries
* da tabela, e um último elemento a NULL.
*/
struct entry_t **table_get_entries(struct table_t *table) {
	int num_entries = table_size(table);

	struct entry_t **entries = (struct entry_t**)
		malloc(sizeof(struct entry_t*) *	(num_entries + 1));

	if (entries == NULL)
		return NULL;

	int index = 0;
	int i;

	//iterar listas
	for (i = 0; i < table->num_places; i++) {
		if (table->places[i] != NULL) {
			struct node_t *curr = table->places[i]->head;

			//iterar nos da lista
			while(curr != NULL) {
				//copiar entry
				entries[index] = entry_dup(curr->entry);

				//proximo elemento da lista
				curr = curr->next;
				index++;
			}
		}
	}

	//terminar array de entries
	entries[num_entries] = NULL;

	return entries;
}


/* Liberta a memória alocada por table_get_entries().
*/
void table_free_entries(struct entry_t **entries) {

	int i;

	for (i = 0; entries[i] != NULL; i++) {
		entry_destroy(entries[i]);
	}

	free(entries);
}


long long table_get_ts(struct table_t *table, char *key) {

	if (table == NULL || key == NULL)
		return -1;

	struct data_t *data = table_get(table, key);

	if (data == NULL)
		return 0;

	data_destroy(data);

	return data->timestamp;
}

