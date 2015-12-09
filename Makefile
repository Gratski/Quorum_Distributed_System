#  	-- Grupo 3 --
#	João Gouveia 	nº 45679
#	João Rodrigues	nº 45582
#	Pedro Luís 	nº 45588

CC = gcc
O_FILES = object
C_FILES = source
H_FILES = include
CFLAGS = -g -Wall -I$(H_FILES)

default: all

all: table_client table_server table_client_test

clean:
	rm -f table_client
	rm -f table_server
	rm -f table_client_test
	rm -f test_queue
	rm -f object/*.o
	rm -f *~
	rm -f source/*~

clean_log:
	rm -f *.log


table_client: $(O_FILES)/table_client.o $(O_FILES)/rqueue.o $(O_FILES)/quorum_table.o $(O_FILES)/quorum_access.o $(O_FILES)/client_stub.o $(O_FILES)/message.o $(O_FILES)/network_client.o $(O_FILES)/data.o $(O_FILES)/entry.o
	$(CC) $(CFLAGS) -pthread -o table_client $(O_FILES)/rqueue.o $(O_FILES)/table_client.o $(O_FILES)/quorum_table.o $(O_FILES)/quorum_access.o $(O_FILES)/client_stub.o $(O_FILES)/message.o $(O_FILES)/network_client.o $(O_FILES)/data.o $(O_FILES)/entry.o

table_server: $(O_FILES)/table_server.o $(O_FILES)/table_skel.o $(O_FILES)/persistence_manager.o $(O_FILES)/persistent_table.o $(O_FILES)/table.o $(O_FILES)/message.o $(O_FILES)/list.o $(O_FILES)/entry.o $(O_FILES)/data.o $(O_FILES)/rqueue.o
	$(CC) $(CFLAGS) -o table_server $(O_FILES)/table_server.o $(O_FILES)/table_skel.o $(O_FILES)/persistence_manager.o $(O_FILES)/persistent_table.o $(O_FILES)/table.o $(O_FILES)/message.o $(O_FILES)/list.o $(O_FILES)/entry.o $(O_FILES)/data.o $(O_FILES)/rqueue.o

test_queue: $(O_FILES)/test_queue.o $(O_FILES)/rqueue.o
	$(CC) $(CFLAGS) -o test_queue $(O_FILES)/test_queue.o $(O_FILES)/rqueue.o

table_client_test: $(O_FILES)/table_client_test.o $(O_FILES)/rqueue.o $(O_FILES)/quorum_table.o $(O_FILES)/quorum_access.o $(O_FILES)/client_stub.o $(O_FILES)/message.o $(O_FILES)/network_client.o $(O_FILES)/data.o $(O_FILES)/entry.o
	$(CC) $(CFLAGS) -pthread -o table_client_test $(O_FILES)/rqueue.o $(O_FILES)/table_client_test.o $(O_FILES)/quorum_table.o $(O_FILES)/quorum_access.o $(O_FILES)/client_stub.o $(O_FILES)/message.o $(O_FILES)/network_client.o $(O_FILES)/data.o $(O_FILES)/entry.o


# Structs
$(O_FILES)/table_client_test.o: $(C_FILES)/table_client_test.c $(H_FILES)/table_client_test-private.h $(H_FILES)/data-private.h $(H_FILES)/message.h $(H_FILES)/network_client.h $(H_FILES)/quorum_table-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/table_client_test.c -o $(O_FILES)/table_client_test.o

$(O_FILES)/test_queue.o: $(C_FILES)/test_queue.c $(H_FILES)/rqueue.h
	$(CC) $(CFLAGS) -c $(C_FILES)/test_queue.c -o $(O_FILES)/test_queue.o

$(O_FILES)/rqueue.o: $(C_FILES)/rqueue.c $(H_FILES)/rqueue.h
	$(CC) $(CFLAGS) -c $(C_FILES)/rqueue.c -o $(O_FILES)/rqueue.o

$(O_FILES)/quorum_table.o: $(C_FILES)/quorum_table.c $(H_FILES)/quorum_table-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/quorum_table.c -o $(O_FILES)/quorum_table.o

$(O_FILES)/quorum_access.o: $(C_FILES)/quorum_access.c $(H_FILES)/quorum_access.h $(H_FILES)/quorum_table-private.h $(H_FILES)/rqueue.h $(H_FILES)/client_stub-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/quorum_access.c -o $(O_FILES)/quorum_access.o

$(O_FILES)/table_client.o: $(C_FILES)/table_client.c $(H_FILES)/table_client-private.h $(H_FILES)/message-private.h $(H_FILES)/network_client.h $(H_FILES)/quorum_table-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/table_client.c -o $(O_FILES)/table_client.o

$(O_FILES)/table_server.o: $(C_FILES)/table_server.c $(H_FILES)/persistent_table-private.h $(H_FILES)/table-private.h $(H_FILES)/message-private.h $(H_FILES)/inet.h $(H_FILES)/table_skel.h $(H_FILES)/table_skel-private.h $(H_FILES)/rqueue.h
	$(CC) $(CFLAGS) -c $(C_FILES)/table_server.c -o $(O_FILES)/table_server.o

$(O_FILES)/network_client.o: $(C_FILES)/network_client.c $(H_FILES)/network_client-private.h $(H_FILES)/message-private.h $(H_FILES)/inet.h
	$(CC) $(CFLAGS) -c $(C_FILES)/network_client.c -o $(O_FILES)/network_client.o

$(O_FILES)/table_skel.o: $(C_FILES)/table_skel.c $(H_FILES)/persistent_table-private.h $(H_FILES)/persistence_manager-private.h $(H_FILES)/table_skel.h $(H_FILES)/table_skel-private.h $(H_FILES)/message-private.h $(H_FILES)/table-private.h $(H_FILES)/inet.h $(H_FILES)/client_stub-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/table_skel.c -o $(O_FILES)/table_skel.o

$(O_FILES)/persistent_table.o: $(C_FILES)/persistent_table.c $(H_FILES)/persistent_table-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/persistent_table.c -o $(O_FILES)/persistent_table.o

$(O_FILES)/persistence_manager.o: $(C_FILES)/persistence_manager.c $(H_FILES)/persistence_manager-private.h $(H_FILES)/data-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/persistence_manager.c -o $(O_FILES)/persistence_manager.o

$(O_FILES)/data.o: $(C_FILES)/data.c $(H_FILES)/data.h
	$(CC) $(CFLAGS) -c $(C_FILES)/data.c -o $(O_FILES)/data.o

$(O_FILES)/entry.o: $(C_FILES)/entry.c $(H_FILES)/data.h $(H_FILES)/entry.h
	$(CC) $(CFLAGS) -c $(C_FILES)/entry.c -o $(O_FILES)/entry.o

$(O_FILES)/list.o: $(C_FILES)/list.c $(H_FILES)/list-private.h $(H_FILES)/entry.h $(H_FILES)/data-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/list.c -o $(O_FILES)/list.o

$(O_FILES)/table.o: $(C_FILES)/table.c $(H_FILES)/list-private.h $(H_FILES)/table-private.h $(H_FILES)/data-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/table.c -o $(O_FILES)/table.o

$(O_FILES)/message.o: $(C_FILES)/message.c $(H_FILES)/data.h $(H_FILES)/entry.h $(H_FILES)/message-private.h $(H_FILES)/client_stub-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/message.c -o $(O_FILES)/message.o

$(O_FILES)/client_stub.o: $(C_FILES)/client_stub.c $(H_FILES)/client_stub-private.h $(H_FILES)/network_client.h $(H_FILES)/network_client-private.h $(H_FILES)/data.h $(H_FILES)/entry.h $(H_FILES)/message-private.h
	$(CC) $(CFLAGS) -c $(C_FILES)/client_stub.c -o $(O_FILES)/client_stub.o
