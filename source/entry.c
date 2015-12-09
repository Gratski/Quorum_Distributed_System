/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#include "entry.h"
#include "data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct entry_t *entry_create(char *key, struct data_t *data){
	if (key == NULL)
		return NULL;

	struct entry_t *e = (struct entry_t *) malloc(sizeof(struct entry_t));
	e->key   = strdup(key);
	e->value = data_dup(data);
	return e;
}

void entry_destroy(struct entry_t *entry){
	if (entry == NULL)
		return;

	// libertar data da entry
	data_destroy(entry->value);

	// libertar key
	if (entry->key != NULL)
		free(entry->key);

	// libertar entry
	free(entry);
}

struct entry_t *entry_dup(struct entry_t *entry){
	if (entry == NULL)
		return NULL;

	// cria nova struct
	return entry_create(entry->key, entry->value);

}