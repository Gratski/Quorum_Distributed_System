/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H

#include "data.h"
#include "entry.h"
#include "list.h"

/* No de uma lista ligada */
struct node_t{
	struct entry_t *entry;	// entry
	struct node_t *next;		// proximo no da lista
};

/* ver list.h doc */
struct list_t{
	int size;					// numero de entradas da lista
	struct node_t *head;		// head da lista
};


/* Imprime lista no ecra */
void list_print(struct list_t *list);

#endif