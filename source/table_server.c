/*    -- Grupo 3 --
   João Gouveia   nº 45679
   João Rodrigues nº 45582
   Pedro Luís     nº 45588
*/

#include <signal.h>
#include <unistd.h>

#include "inet.h"
#include "poll.h"
#include "fcntl.h"
#include "table-private.h"
#include "message-private.h"
#include "table_skel-private.h"
#include "persistent_table-private.h"
#include "persistence_manager-private.h"
#include "rqueue.h"

/*
 * A ação associada à receção de SIGPIPE passa a ser ignorada.
 */
int ignsigpipe(){
   struct sigaction s;

   s.sa_handler = SIG_IGN;
   return sigaction(SIGPIPE, &s, NULL);
}

int main(int argc, char **argv){
   int sockfd;
   struct sockaddr_in client;
   socklen_t size_client;

   struct pollfd sockets[NUMFD];
   int num_connected = 1;
   puts("1");
   //validacao de argumentos
   if( argc != 4 )
   {
      perror("Exemplo de uso: ./table-server <porto> <tamanho da table> <nome ficheiro de log>\n");
      return -1;
   }

   //ignorar sinais do tipo SIGPIPE
   if (ignsigpipe() != 0){
      perror("ignsigpipe falhou");
      exit(1);
   }

   // leitura de params
   int port       = atoi(argv[1]);
   int n_lists    = atoi(argv[2]);
   char* log_file = argv[3];

   if (port < 1024) {
      puts("Valor do porto tem de ser superior a 1024!");
      return -1;
   }

   if (n_lists < 1) {
      puts("Tamanho da tabela tem de ser superior a 0!");
      return -1;
   }

   if (log_file == NULL) {
      puts("Por favor especifique um nome para o ficheiro de log");
      return -1;
   }

   if( table_skel_init(n_lists, log_file, 50) < 0 )
   {
      puts("Nao foi possivel inicializar a table!");
      return -1;
   }
   else
      puts("\nTable inicializada!");

   //iniciar ligacao
   struct sockaddr_in server;

   //criar socket TCP
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
      perror("Erro ao criar socket");
      return -1;
   }

   server.sin_port        = port;
   server.sin_addr.s_addr = htonl(INADDR_ANY);
   server.sin_family      = AF_INET;

   int enable = 1;

   if( ( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) ) < 0 )
   {
      perror("Erro ao tornar porto reutilizavel");
      return -1;
   }

   //binding de socketdf com server
   if( (bind(sockfd, (struct sockaddr *) &server, sizeof(server))) < 0 )
   {
      perror("Erro a fazer bind de socket com server");
      close(sockfd);
      return -1;
   }

   //declarar sockfd como socket de escuta
   if( listen(sockfd, 0) < 0 )
   {
      perror("Error ao colocar á escuta");
      close(sockfd);
      return -1;
   }

   //feedback para terminal
   puts("\nServidor pronto e à escuta...");

   // nenhuma ligacao esta aberta
   int i = 0;

   for (i = 1; i < NUMFD; i++)
      sockets[i].fd = -1;

   // socket de entrada de ligacoes
   sockets[0].fd = sockfd;
   sockets[0].events = POLLIN;
   num_connected = 1;

   size_client = sizeof(client);
   int fd_to_read = 0;

   while( ( fd_to_read = poll( sockets, NUMFD, 10) ) >= 0 )
   {
      //se ha algo I/O
      if( fd_to_read > 0 )
      {
         //se alguem se ligou
         if( ( sockets[0].revents & POLLIN ) && ( num_connected < NUMFD ) )
         {
            puts("\nNovo client ligado!");
            // encontra primeira posicao livre
            int pos = 1;
            int index = 1;

            for (index = 1; index < NUMFD; index++)
            {
               if( sockets[index].fd == -1 )
               {
                  pos = index;
                  break;
               }
            }

            // insere na lista de sockets
            if( ( sockets[pos].fd = accept( sockets[0].fd, (struct sockaddr *) &client, &size_client ) ) > 0 )
            {
               sockets[pos].events = POLLIN;
               num_connected++;
            }
         }

         int index = 1;

         // executa pedidos para cada cliente que pediu algo
         for (index = 1; index < NUMFD; index++)
         {
            // se tem dados para ler
            if( (sockets[index].revents & POLLIN) && ( sockets[index].fd != -1 ) )
            {
               puts("\nPedido de client");
               struct message_t *request = network_receive( sockets[index].fd );

               if ( request == NULL )
               {
                  close(sockets[index].fd);
                  sockets[index].fd = -1;
                  num_connected--;
                  printf("\nClient %d terminou ligacao\n", index);
                  fflush(stdout);
                  continue;
               }

               //processar pedido
               struct message_t *server_rsp = invoke(request);

               //enviar resposta
               int result_code = network_send( sockets[index].fd, server_rsp );

               // se o client fechou a ligação a meio
               if(result_code < 0)
               {
                  close(sockets[index].fd);
                  sockets[index].fd = -1;
                  num_connected--;
                  puts("\nClient fechou ligacao");
                  continue;
               }
               // se processou bem
               else
               {
                  printf("\nClient %d feito!\n", index);
               }
            }
         }
      }
   }

   //fechar socket
   close(sockfd);
   table_skel_destroy(NULL);

   return 0;
}


struct message_t *network_receive(int connsockfd) {

   char *buffer_string = NULL;

   //ler tamanho da mensagem
   //cliente terminou ligacao?
   if (read_all(connsockfd, &buffer_string, INT_SIZE) == 0) {
      return NULL;
   }

   //converter tamanho da mensagem
   int msg_size;
   memcpy(&msg_size, buffer_string, 4);
   msg_size = ntohl(msg_size);
   free(buffer_string);

   //receber mensagem
   //cliente terminou ligacao?
   if (read_all(connsockfd, &buffer_string, msg_size) == 0) {
      return NULL;
   }

   //converter mensagem do cliente
   struct message_t *msg = buffer_to_message(buffer_string, msg_size);
   free(buffer_string);

   if (msg == NULL) {
      perror("table_server.c - Mensagem invalida!");
      return NULL;
   }

   return msg;
}


int network_send(int connsockfd, struct message_t *msg) {

   if (msg == NULL) {
      puts("network_send - Msg eh NULL!");
      return -1;
   }

   char *buffer_string = NULL;

   //preparar buffer a enviar
   int buffer_size = message_to_buffer(msg, &buffer_string);

   //converter formatos
   int buffer_size_htonl = htonl(buffer_size);
   char *buffer_size_char = (char *) malloc(4);
   memcpy(buffer_size_char, &buffer_size_htonl, 4);

   //enviar tamanho da mensagem
   write_all(connsockfd, buffer_size_char, 4);
   free(buffer_size_char);

   //enviar mensagem
   write_all(connsockfd, buffer_string, buffer_size);
   free(buffer_string);

   return 0;
}



