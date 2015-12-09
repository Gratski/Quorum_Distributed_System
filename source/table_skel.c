/*    -- Grupo 3 --
   João Gouveia   nº 45679
   João Rodrigues nº 45582
   Pedro Luís     nº 45588
*/

#include "inet.h"
#include "table-private.h"
#include "message-private.h"
#include "client_stub-private.h"
#include "persistent_table-private.h"
#include "persistence_manager-private.h"
#include "table_skel-private.h"

static struct ptable_t *ptable = NULL;

int table_skel_init(int n_lists, char *filename, int logsize) {
   if( n_lists < 0 )
      return -1;

   struct table_t *table = table_create(n_lists);

   if( table == NULL )
   {
      puts("Erro ao criar table");
      return -1;
   }

   struct pmanager_t *pmanager = pmanager_create(filename, logsize);

   if( pmanager == NULL )
   {
      puts("Erro ao criar pmanager");
      return -1;
   }

   ptable = ptable_open( table, pmanager );

   // preenche a table com todos os dados ja existentes
   if( pmanager_fill_state(ptable->pmanager, ptable->table) >= 0 ) {
      puts("\nFicheiro de log carregado com sucesso!");
   } else {
      puts("\nTable inicializada vazia");
   }

   return 0;
}


int table_skel_destroy() {
   if( ptable == NULL )
      return -1;

   //fclose(ptable->pmanager->fd);
   table_destroy(ptable->table);
   pmanager_destroy_clear(ptable->pmanager);
   free(ptable);

   return 0;
}


struct message_t *invoke(struct message_t *msg_in) {

   if (msg_in == NULL || ptable == NULL)
      return NULL;

   struct message_t *server_msg;

   //executar instrucoes do cliente
   switch(msg_in->opcode) {
      //PUT
      case OC_PUT:
         puts("\n- PUT -");
         server_msg = exec_put_msg(ptable, msg_in);
         break;

      //GET
      case OC_GET:
         if ( msg_in->content.key == NULL )
         {
            server_msg = get_error_msg();
         }

         //GET KEYS
         else if (strcmp(msg_in->content.key, "!") == 0)
         {
            puts("\n- GET KEYS -");
            server_msg = exec_get_keys_msg(ptable->table);

         //GET KEY
         } else
         {
            puts("\n- GET KEY -");
            server_msg = exec_get_msg(ptable->table, msg_in);
         }

         break;

      //DEL
      case OC_DEL:
         puts("\n- OC DEL -");
         server_msg = exec_del_msg(ptable->table, msg_in);
         break;

      //UPDATE
      case OC_UPDATE:
         puts("\n- OC UPDATE -");
         server_msg = exec_update_msg(ptable->table, msg_in);
         break;

      //SIZE
      case OC_SIZE:
         puts("\n- OC SIZE -");
         server_msg = exec_get_size_msg(ptable->table);
         break;

      //NUM OPS
      case OC_NUM_OPS:
         puts("\n- OC NUM OPS -");
         server_msg = exec_get_num_change_ops(ptable->table);
         break;

      //TIMESTAMP
      case OC_RT_GETTS:
         puts("\n- OC TIMESTAMP -");
         server_msg = exec_get_timestamp(ptable->table, msg_in);
         printf("Timestamp: %lld\n", server_msg->content.timestamp);
         break;

      //opcode invalido
      default:
         server_msg = get_error_msg();
         break;
   }

   return server_msg;
}


struct message_t *get_error_msg() {
   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL)
   {
      perror("Falta de memoria!");
      return NULL;
   }

   msg->opcode         = OC_RT_ERROR;
   msg->c_type         = CT_RESULT;
   msg->content.result = -1;

   return msg;
}


char *prepare_log_entry(struct message_t *msg) {

   if (msg == NULL)
      return NULL;

   char *file_buffer = NULL;
   int buffer_size = message_to_buffer(msg, &file_buffer);

   char *aux_buffer = (char *) malloc(INT_SIZE + buffer_size);
   memcpy(aux_buffer, &buffer_size, INT_SIZE);
   memcpy(aux_buffer + INT_SIZE, file_buffer, buffer_size);

   return(aux_buffer);
}


int check_log_size(struct ptable_t *ptable, struct message_t *client_msg) {

   if (ptable == NULL || client_msg == NULL)
      return -1;

   char* log_entry = prepare_log_entry(client_msg);

   if ( log_entry != NULL )
   {
      int entry_length;
      memcpy(&entry_length, log_entry, INT_SIZE);

      //ficheiro de log ira ultrapassar o seu limite
      if( ptable->pmanager->file_size + INT_SIZE + entry_length >
         ptable->pmanager->max_file_size )
      {
         puts("\nFicheiro de log cheio.");
         //criar novo ficheiro de checkpoint
         if ((pmanager_store_table(ptable->pmanager, ptable->table) > 0) &&
            pmanager_rotate_log(ptable->pmanager) == 0)
         {
            puts("\nCriado novo ficheiro de checkpoint.");
         }
      }
      //erro ao adicionar entrada ao log
      else if ( pmanager_log(ptable->pmanager, log_entry) < 0 )
      {
         puts("Logfile nao actualizado!");
         return -1;
      }
   }
   else
   {
      puts("Erro ao criar entry do log");
      return -1;
   }

   free(log_entry);

   return 0;
}


struct message_t *exec_put_msg(struct ptable_t *ptable,
   struct message_t *client_msg) {

   if (ptable == NULL || client_msg == NULL)
      return NULL;

   int result = table_put( ptable->table, client_msg->content.entry->key,
      client_msg->content.entry->value );

   if (result == -1){
      puts("Erro ao introduzir entry!");
      return get_error_msg();
   }

   puts("Entry introduzida!");
   printf("Key: %s | Value: %s | Timestamp: %lld\n",
      client_msg->content.entry->key,
      (char*) client_msg->content.entry->value->data,
      client_msg->content.entry->value->timestamp);

   //verificar se o tamanho do log foi ultrapassado
   check_log_size(ptable, client_msg);

   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL) {
      return NULL;
   }

   msg->opcode = OC_PUT + 1;
   msg->c_type = CT_RESULT;

   return msg;
}


struct message_t *exec_get_msg(struct table_t *table, struct message_t *client_msg) {

   if (table == NULL || client_msg == NULL)
      return NULL;

   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL)
      return NULL;

   msg->opcode       = OC_GET + 1;
   msg->c_type       = CT_VALUE;
   msg->content.data = table_get(table, client_msg->content.key);

   if( msg->content.data == NULL )
   {
      puts("Entry nao encontrada!");
      return get_error_msg();
   }

   puts("Entry encontrada!");
   printf("Key: %s | Value: %s | Timestamp: %lld\n",
      client_msg->content.key, (char*) msg->content.data->data,
      msg->content.data->timestamp);

   return msg;
}


struct message_t *exec_get_keys_msg(struct table_t *table) {
   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL)
      return NULL;

   msg->opcode       = OC_GET + 1;
   msg->c_type       = CT_KEYS;
   msg->content.keys = ptable_get_keys(ptable);

   if (msg->content.keys[0] == NULL) {
      puts("Nao existem entries adicionadas!");
   } else {
      int i;

      for (i = 0; msg->content.keys[i] != NULL; i++) {
         printf("Key %d: %s\n", i + 1, msg->content.keys[i]);
      }
   }

   return msg;
}


struct message_t *exec_del_msg(struct table_t *table, struct message_t *client_msg) {
   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL)
      return NULL;

   //colocar timestamp = 0
   //int result = table_del(table, client_msg->content.key);
   int result = table_update(table, client_msg->content.entry->key, 
      client_msg->content.entry->value);
   
   if (result == -1) {
      puts("Entry nao existe!");
      return get_error_msg();
   }
   else {
      printf("Key %s eliminada com sucesso!\n", client_msg->content.key);

      msg->opcode         = OC_DEL + 1;
      msg->c_type         = CT_RESULT;
      msg->content.result = result;

      check_log_size(ptable, client_msg);
   }

   return msg;
}


struct message_t *exec_update_msg(struct table_t *table, struct message_t *client_msg) {
   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL)
      return NULL;

   msg->opcode = OC_UPDATE + 1;
   msg->c_type = CT_RESULT;

   msg->content.result = table_update(table,
      client_msg->content.entry->key, client_msg->content.entry->value);

   if (msg->content.result == -1) {
      perror("Erro ao atualizar table!");
      return get_error_msg();

   } else {
      puts("Entry atualizada com sucesso!");
      printf("Key: %s | Value: %s\n", client_msg->content.entry->key, (char*)
         client_msg->content.entry->value->data);

      //verificar se o tamanho do log foi ultrapassado
      check_log_size(ptable, client_msg);
   }

   return msg;
}


struct message_t *exec_get_size_msg(struct table_t *table) {
   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL)
      return NULL;

   printf("Numero de entradas da tabela: %d\n", table_size(table));
   msg->opcode         = OC_SIZE + 1;
   msg->c_type         = CT_RESULT;
   msg->content.result = table_size(table);

   return msg;
}


struct message_t *exec_get_num_change_ops(struct table_t *table) {
   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL)
      return NULL;

   printf("Numero de operacoes da tabela: %d\n",
      table_get_num_change_ops(table));

   msg->opcode         = OC_NUM_OPS + 1;
   msg->c_type         = CT_RESULT;
   msg->content.result = table_get_num_change_ops(table);

   return msg;
}


/* key nao existir (data == NULL) devolve timestamp = 0
 * key nao existir devolve -1
*/
struct message_t *exec_get_timestamp(struct table_t *table, struct message_t *client_msg) {

   if (table == NULL || client_msg == NULL)
      return NULL;

   struct message_t *msg =
      ( struct message_t* ) malloc( sizeof(struct message_t) );

   if (msg == NULL)
      return NULL;

   msg->opcode = OC_RT_GETTS + 1;
   msg->c_type = CT_TIMESTAMP;

   long long timestamp = table_get_ts(table, client_msg->content.key);

   //key nao existe
   if (timestamp == 0)
      msg->content.timestamp = -1;
   //key existe
   else
      msg->content.timestamp = timestamp;

   return msg;

}
