#!/bin/bash

gcc server.c -o server
gcc client.c -o client
gcc client2.c -o client2

gcc -g -o server2.o -c server2.c -I/usr/local/include

gcc -g -o server2 server2.o -levent -lpthread -lm -levent_pthreads

rm -rf *.o

