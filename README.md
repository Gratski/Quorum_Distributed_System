# Quorum_Distributed_System

#### This distributed system was implemented when i was in my third year of Computer Sciences Degree.

The idea was to create a dynamic distributed system with the inteligence to know the next better server to use when one that currently belongs to the quorum fails.
To execute it:\n 
1. make\n
2. ./table_server 8080 10 backup1  "in one terminal"
3. ./table_server 8081 10 backup2  "in another terminal"
4. ./table_server 8082 10 backup3  "in another terminal"
Now that you have the servers listening you can now execute a client program
5. ./table_client 1 127.0.0.1:8080 127.0.0.1:8081 127.0.0.1:8082   "and this is going to be a client of floor(n / 2) + 1 servers"
6. Execute a few commands and then test if you crash a server if another becomes part of quorum :)

#### NOTE:
###### Please note that this system can always be improved either in terms of code or concept.
