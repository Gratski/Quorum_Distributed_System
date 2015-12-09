/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#include "data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct data_t *data_create(int size){

	// se size negativo
	if (size < 0)
		return NULL;

	struct data_t *d = (struct data_t *) malloc(sizeof( struct data_t ));

	// se nao tem memoria suficiente
	if ( d == NULL )
		return NULL;

	// se conseguiu alocar
	d->datasize = size;

	if (size > 0)
		d->data = malloc(size);
	else
		d->data = NULL;

	return d;

}


struct data_t *data_create2(int size, void* data){
	if (size < 0)
		return NULL;

	struct data_t *d = data_create(size);

	if ( d == NULL )
		return NULL;

	if (size > 0 && data != NULL)
		memcpy(d->data, data, size);

	d->timestamp = 0;

	return d;
}


struct data_t *data_create3(int size, void * data, long long timestamp){
	if (size < 0 || timestamp < 0)
		return NULL;

	struct data_t *d = data_create2(size, data);

	// se nao tem memoria suficiente
	if ( d == NULL )
		return NULL;

	d->timestamp = timestamp;

	return d;

}


void data_destroy(struct data_t *data){
	if (data == NULL)
		return;

	free(data->data);
	free(data);

}


struct data_t *data_dup(struct data_t *data){
	if (data == NULL || data->datasize < 0)
		return NULL;

	return data_create3(data->datasize, data->data, data->timestamp);

}


long long inc_timestamp(long long timestamp, int id) {

	if (timestamp < 0 || id < 0)
		return -1;

	timestamp = (long long) (timestamp / 1000);
	timestamp = ((timestamp + 1) * 1000) + id;

	return timestamp;
}
