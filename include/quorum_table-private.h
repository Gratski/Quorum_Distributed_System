#ifndef _QUORUM_TABLE_PRIVATE_H
#define _QUORUM_TABLE_PRIVATE_H

#include "quorum_table.h"

struct qtable_t {
	int id; //id do emissor
	int operation;
	int num_tables;
	struct rtable_t *rtables;
};

/*
	Gera um timestamp
*/
long ts_create(int id);


#endif
