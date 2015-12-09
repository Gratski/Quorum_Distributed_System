/* 	-- Grupo 3 --
	João Gouveia 	nº 45679
	João Rodrigues	nº 45582
	Pedro Luís 		nº 45588
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "inet.h"
#include "data-private.h"
#include "persistence_manager-private.h"
#include "table-private.h"
#include "message-private.h"

/*
 * Cria um gestor de persistência que armazena logs em filename+".log".
 * O parâmetro logsize define o tamanho máximo em bytes que o ficheiro de
 * log pode ter.
 * Note que filename pode ser um path completo. Retorna o pmanager criado
 * ou NULL em caso de erro.
 */
struct pmanager_t *pmanager_create(char *filename, int logsize) {

	if (filename == NULL || logsize <= 0)
		return NULL;

	struct pmanager_t *pmanager = (struct pmanager_t*) malloc(sizeof(struct pmanager_t));

	if (pmanager == NULL)
		return NULL;

	//guardar nome do ficheiro de log
   pmanager->file_name = strdup(filename);

   if (pmanager->file_name == NULL) {
   	free(pmanager);
   	return NULL;
   }

	pmanager->file_size     = 0;
	pmanager->max_file_size = logsize;

	// abre file streams
   char *log_file_name = append_file_type(filename, ".log");
	struct stat st;

   //ficheiro de log já existe
	if (stat(log_file_name, &st) == 0)
	{
		pmanager->fd = fopen(log_file_name, "ab+");
	//ficheiro de log nao existe
	} else
	{
		pmanager->fd = fopen(log_file_name, "wb");
	}

	if( pmanager->fd == NULL)
	{
		puts("Erro ao abrir file streams!");
		free(pmanager->file_name);
		free(pmanager);

		return NULL;
	}

	// forca escrita em disco
	fflush(pmanager->fd);
	fsync( fileno( pmanager->fd ) );
	fseek(pmanager->fd, 0, SEEK_END);

 	free(log_file_name);

	return pmanager;
}


/* Destrói o gestor de persistência pmanager. Retorna 0 se tudo estiver OK
 * ou -1 em caso de erro. Esta função não limpa o ficheiro de log.
 */
int pmanager_destroy(struct pmanager_t *pmanager) {

	if (pmanager == NULL)
		return -1;

	fclose(pmanager->fd);

	free(pmanager->file_name);
	free(pmanager);

	return 0;
}


/* Apaga o ficheiro de log gerido pelo gestor de persistência.
 * Retorna 0 se tudo estiver OK ou -1 em caso de erro.
 */
int pmanager_destroy_clear(struct pmanager_t *pmanager) {

	if (pmanager == NULL)
		return -1;

	if (remove(pmanager->file_name) != 0)
		return -1;

	return 0;
}


/* Retorna 1 caso existam dados ficheiros de dados.
 */
int pmanager_has_data(struct pmanager_t *pmanager) {

	if (pmanager == NULL)
		return -1;

	struct stat st;
	char *log_file_name = append_file_type(pmanager->file_name, ".log");
	char *ckp_file_name = append_file_type(pmanager->file_name, ".ckp");
	char *stt_file_name = append_file_type(pmanager->file_name, ".stt");

	int log_file_exists = (stat(log_file_name, &st) == 0);
	int ckp_file_exists = (stat(ckp_file_name, &st) == 0);
	int stt_file_exists = (stat(stt_file_name, &st) == 0);

	free(log_file_name);
	free(ckp_file_name);
	free(stt_file_name);

   return (log_file_exists || ckp_file_exists || stt_file_exists);

}


/* Adiciona uma string msg no fim do ficheiro de log associado a pmanager.
 * Retorna o numero de bytes escritos no log ou -1 em caso de problemas na
 * escrita (e.g., erro no write()), ou no caso em que o tamanho do ficheiro
 * de log após o armazenamento da mensagem seja maior que logsize (neste
 * caso msg não é escrita no log).
 */
int pmanager_log(struct pmanager_t *pmanager, char *msg) {

	if (pmanager == NULL || msg == NULL) {
		return -1;
	}

	//obter tamanho da mensagem
	int msg_length;
	memcpy(&msg_length, msg, INT_SIZE);

	//escreve size de msg a ler em binario
	int file_size = fwrite(&msg_length, INT_SIZE, 1, pmanager->fd);
	pmanager->file_size += INT_SIZE;

	//escreve msg em binario
	file_size = fwrite(msg + INT_SIZE, 1, msg_length, pmanager->fd);
	pmanager->file_size += file_size;

	//forca escrita em disco
	fflush(pmanager->fd);
	fsync( fileno( pmanager->fd ) );

	return 0;
}


/* Recupera o estado contido no ficheiro de log, na tabela passada como
 * argumento.
 */
int pmanager_fill_state(struct pmanager_t *pmanager, struct table_t *table) {
	if (pmanager == NULL || table == NULL)
		return -1;

	//nao existe informacao no ficheiro de log
	if ( pmanager_has_data(pmanager) == 0 )
		return -1;

	char *log_file_name = append_file_type(pmanager->file_name, ".log");
	char *ckp_file_name = append_file_type(pmanager->file_name, ".ckp");
	char *stt_file_name = append_file_type(pmanager->file_name, ".stt");

	struct stat st;
	int stt_exists = (stat(stt_file_name, &st) == 0);
	int ckp_exists = (stat(ckp_file_name, &st) == 0);

	//existe um ficheiro stt
	if (stt_exists) {
		//stt completo e log vazio
		//stt tinha acabado de ser criado e chegou a apagar o log
		if (is_complete(stt_file_name) && is_file_empty(log_file_name)) {
			//criar ckp e eliminar stt
			puts("Carregar ficheiro stt.");
			pmanager_rotate_log(pmanager);
		}
		//existe ficheiro stt, existe ficheiro ckp e ficheiro stt nao esta completo
		else if (ckp_exists && !is_complete(stt_file_name)) {
			int log_file_loaded;
			//carregar conteudo a partir dos ficheiros ckp e log
			if ((load_file(ckp_file_name, table) == 0)
				&& (log_file_loaded = load_log_file(log_file_name, table) == 0)) {
				puts("\nCarregar ficheiro de checkpoint");
				//remover ficheiro stt
				remove(stt_file_name);
			//erro ao carregar log
			} else if (log_file_loaded < 0){
				puts("Erro ao carregar ficheiros!");
				free(log_file_name);
				free(ckp_file_name);
				free(stt_file_name);

				//remover ficheiros de checkpoint
				remove(stt_file_name);
				remove(ckp_file_name);

				return -1;
			}
		}
		//ficheiro stt completo
		//substituir ficheiro de checkpoint
		else if (is_complete(stt_file_name)) {
			pmanager_rotate_log(pmanager);
		}
		//ficheiro ckp nao existe
		else {
			pmanager_rotate_log(pmanager);
		}
	}
	//nao existe ficheiro stt mas existe ficheiro ckp
	//carregar conteudo do ficheiro ckp e log
	else if (ckp_exists) {
		//carregar conteudo do ficheiro ckp para a table
		puts("\nCarregar ficheiro ckp");
		if (load_file(ckp_file_name, table) < 0){
			free(log_file_name);
			free(ckp_file_name);
			free(stt_file_name);

			return -1;
		}

		puts("\nCarregar ficheiro de log");
		load_log_file(log_file_name, table);

	//apenas existe ficheiro de log
	} 
	else if (!is_file_empty(log_file_name)) 
	{
		load_log_file(log_file_name, table);
	}

	free(log_file_name);
	free(ckp_file_name);
	free(stt_file_name);

	return 0;
}


int is_complete(char* file_name) {

	if (file_name == NULL)
		return -1;

	FILE *fd = fopen(file_name, "rb");

	if (fd == NULL)
		return -1;

	int file_is_complete = 0;
	fseek(fd, 0, SEEK_SET);
	fread(&file_is_complete, SHORT_SIZE, 1, fd);

	fclose(fd);

	return file_is_complete;
}


int load_log_file(char* file_name, struct table_t *table) {

	struct stat st;

	if (stat(file_name, &st) != 0) {
		free(file_name);
		return -1;
	}

	FILE* fd = fopen(file_name, "rb");

	if (fd == NULL)
		return -1;

	int nbytes = 0;
	int msg_size = 0;

	//ler conteudo do ficheiro de log
	while ( fread(&msg_size, INT_SIZE, 1, fd) > 0 ) {
		nbytes += INT_SIZE;

		char *buffer = (char *) malloc(msg_size);

		if( buffer == NULL ) {
			free(file_name);
			return -1;
		}

   	//ler tamanho da mensagem
   	fread(buffer, 1, msg_size, fd);
   	nbytes += msg_size;

   	//obter tipo de instrucao
   	struct message_t *msg = buffer_to_message(buffer, msg_size);

		//erro ao converter mensagem
		if (msg == NULL) {
			free(buffer);
			puts("Erro ao converter mensagem!");

			return -1;
		}

		//introduzir entry na table
		if (msg->opcode == OC_UPDATE) {
			table_update(table, msg->content.entry->key, msg->content.entry->value);
			printf("UPDATE -> key: %s | value: %s\n", msg->content.entry->key,
			(char *) msg->content.entry->value->data);

		} else {
			table_put(table, msg->content.entry->key, msg->content.entry->value);

			printf("PUT -> key: %s | value: %s\n", msg->content.entry->key,
			(char *) msg->content.entry->value->data);
		}
		
		free(buffer);
		free_message(msg);
	}

	fclose(fd);

	return nbytes;

}


int load_file(char* file_name, struct table_t *table) {

	struct stat st;

	if (stat(file_name, &st) != 0) {
		free(file_name);
		return -1;
	}

	FILE* fd = fopen(file_name, "rb");

	if (fd == NULL)
		return -1;

	short is_complete = 0;
	fread(&is_complete, SHORT_SIZE, 1, fd);

	//ler numero de operacoes de modificacao
	int num_change_ops = 0;
	fread(&num_change_ops, INT_SIZE, 1, fd);

	int msg_size;
	int nbytes = 0;

	//ler conteudo do ficheiro de log
	while ( fread(&msg_size, INT_SIZE, 1, fd) > 0 ) {
		nbytes += INT_SIZE;

		char *buffer = (char *) malloc(msg_size);

		if( buffer == NULL ) {
			free(file_name);
			return -1;
		}

	   	//ler tamanho da mensagem
   		fread(buffer, 1, msg_size, fd);
   		nbytes += msg_size;

   		//obter tipo de instrucao
   		struct message_t *msg = buffer_to_message(buffer, msg_size);

		//erro ao converter mensagem
		if (msg == NULL) {
			free(buffer);
			puts("Erro ao converter mensagem!");

			return -1;
		}

		//introduzir entry na table
		table_put(table, msg->content.entry->key, msg->content.entry->value);

		printf("PUT -> key: %s | value: %s\n", msg->content.entry->key,
			(char *) msg->content.entry->value->data);

		free(buffer);
		free_message(msg);
	}

	table->num_change_ops = num_change_ops;
	fclose(fd);

	return nbytes;
}


int is_file_empty(char* file_name) {
	struct stat st;

	//verificar se o ficheiro existe
	if (stat(file_name, &st) != 0)
		return -1;

	FILE *fd = fopen(file_name, "rb");

	if (fd == NULL)
		return -1;

	fseek(fd, 0, SEEK_END);

	if (ftell(fd) == 0) {
		return 1;
	}

	fclose(fd);

	return 0;
}


int file_size(char *file_name) {
	struct stat st;

	//verificar se o ficheiro existe
	if (stat(file_name, &st) != 0)
		return -1;

	FILE *fd = fopen(file_name, "rb");

	if (fd == NULL) {
		return -1;
	}

	int file_size = 0;
	int nbytes = fread(&file_size, INT_SIZE, 1, fd);

	if (nbytes < 0)
		return -1;

	return file_size;
}


char *append_file_type(char *file_name, char* file_type) {

	char *str = (char *) malloc(strlen(file_name) + strlen(file_type) + 1);

	if (str == NULL) {
		return NULL;
	}

	strcpy(str, file_name);
	strcat(str, file_type);

	return str;
}


/* Cria um ficheiro filename+".stt" com o estado atual da tabela table.
* Retorna o tamanho do ficheiro criado ou -1 em caso de erro.
*/
int pmanager_store_table(struct pmanager_t *pmanager, struct table_t *table) {

	char *stt_file_name = append_file_type(pmanager->file_name, ".stt");
	FILE *stt_fd = fopen(stt_file_name, "wb");

	if (stt_fd == NULL)
		return -1;

	struct entry_t **entries = table_get_entries(table);
	if( entries == NULL )
		return -1;

	puts("\nCriar ficheiro de checkpoint.");

	//inicializar posicao relativa ao estado final
	//do ficheiro (completo/incompleto) a 0 (incompleto)
	short file_complete = 0;
	fwrite(&file_complete, SHORT_SIZE, 1, stt_fd);

	//guardar numero de operacoes de modificacao
	int num_change_ops = table_get_num_change_ops(table);
	fwrite(&num_change_ops, INT_SIZE, 1, stt_fd);

	//guardar entries no ficheiro
	int i, file_size = 0;
	for (i = 0; entries[i] != NULL; i++) {

		char* msg_buff = NULL;
		struct message_t *msg = (struct message_t*) malloc(sizeof(struct message_t));

		//criar mensagem a ser guardada
		msg->opcode = OC_PUT;
		msg->c_type = CT_ENTRY;
		msg->content.entry = entry_dup(entries[i]);

		//serializar mensagem
		int entry_size = message_to_buffer(msg, &msg_buff);

		//guardar tamanho da mensagem
		fwrite(&entry_size, INT_SIZE, 1, stt_fd);
		file_size += INT_SIZE;

		//guardar mensagem
		fwrite(msg_buff, 1, entry_size, stt_fd);
		file_size += entry_size;

		free_message(msg);
		free(msg_buff);
	}

	//inicializar posicao relativa ao estado final
	//do ficheiro (completo/incompleto) a 1 (completo)
	file_complete = 1;
	fseek( stt_fd, 0, SEEK_SET );
	fwrite( &file_complete, SHORT_SIZE, 1, stt_fd );

	fclose(stt_fd);

	//ficheiro stt gravado
	//limpar conteudo do ficheiro de log (overwrite)
	char *log_file_name = append_file_type(pmanager->file_name, ".log");
	FILE *log_fd = fopen(log_file_name, "wb");

	if (log_fd == NULL)
		return -1;

	//reset ao tamanho do ficheiro de log
	pmanager->file_size = SHORT_SIZE + INT_SIZE;
	fclose(log_fd);

	free(log_file_name);

	puts("\nFicheiro stt gravado com sucesso.");

	return file_size;
}


/* Limpa o conteúdo do ficheiro ".log" e copia o ficheiro ".stt" para ".ckp".
* Retorna 0 se tudo correr bem ou -1 em caso de erro.
*/
int pmanager_rotate_log(struct pmanager_t *pmanager) {

	if (pmanager == NULL)
		return -1;

	puts("\nRotating log.");

	char *stt_file_name = append_file_type(pmanager->file_name, ".stt");
	char *ckp_file_name = append_file_type(pmanager->file_name, ".ckp");
	char *log_file_name = append_file_type(pmanager->file_name, ".log");

	struct stat st;

	//verificar se ficheiro stt existe
	if (stat(stt_file_name, &st) != 0)
		return -1;

	//abrir ficheiro stt para leitura
	FILE *stt_fd = fopen(stt_file_name, "rb");

	if (stt_fd == NULL)
		return -1;

	//abrir ficheiro de checkpoint para escrita
	FILE *ckp_fd = fopen(ckp_file_name, "wb");

	if (ckp_fd == NULL) {
		fclose(stt_fd);
		return -1;
	}

  	//inicializar estado do ficheiro
  	//este valor apenas e alterado apos ter sido gravado com sucesso
  	int file_state = 0;
  	fwrite(&file_state, SHORT_SIZE, 1, ckp_fd);

  	//copiar numero de operacoes de modificacao
  	int num_change_ops = 0;
	fseek(stt_fd, SHORT_SIZE, SEEK_SET);
	fread(&num_change_ops, INT_SIZE, 1, stt_fd);
  	fwrite(&num_change_ops, INT_SIZE, 1, ckp_fd);

  	//posicionar ficheiros para copiar os seus respectivos valores
	int buff_size;
	int ckp_file_size = 0;
	char* key         = NULL;
	char* buff_msg    = NULL;
	void* data        = NULL;

	fseek( stt_fd, SHORT_SIZE + INT_SIZE, SEEK_SET );
	fseek( ckp_fd, SHORT_SIZE + INT_SIZE, SEEK_SET );

	//copiar conteudo do ficheiro stt para o ficheiro ckp
	while (fread(&buff_size, INT_SIZE, 1, stt_fd) > 0){
		//guardar tamanho da mensagem
	   	fwrite(&buff_size, INT_SIZE, 1, ckp_fd);
   		ckp_file_size += INT_SIZE;

		//ler mensagem
		buff_msg = (char*) malloc(buff_size);
		fread(buff_msg, 1, buff_size, stt_fd);

		//guardar mensagem
	   	fwrite(buff_msg, 1, buff_size, ckp_fd);
   		ckp_file_size += buff_size;

   		free(buff_msg);
   		free(key);
   		free(data);
 	}

 	//marcar ficheiro ckp como completo
 	file_state = 1;
	fseek( ckp_fd, 0, SEEK_SET );
	fwrite( &file_state, SHORT_SIZE, 1, ckp_fd );

 	fclose(stt_fd);
 	fclose(ckp_fd);

	//eliminar ficheiro stt
	remove(stt_file_name);

	//libertar memoria
	free(stt_file_name);
	free(ckp_file_name);
	free(log_file_name);

 	return ckp_file_size;
}
