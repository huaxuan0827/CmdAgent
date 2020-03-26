#!/bin/bash

# build devnet_ut.
gcc -g -o devnet.o -c ../../src/agent/devnet.c -I../../src/agent -I../../src
gcc -g -o SimuList.o -c ../../src/agent/SimuList.c -I../../src/agent -I../../src
gcc -g -o devnet_ut.o -c devnet_ut.c -I../../src/agent -I../../src

gcc -g -o devnet_ut devnet.o SimuList.o devnet_ut.o -ll0001-0 -ll0002-0 -ll0003-0 -levent -lpthread -lm

# build sernet_ut.

gcc -g -o sernet.o -c ../../src/agent/sernet.c -I../../src/agent -I../../src
gcc -g -o serclient.o -c ../../src/agent/serclient.c -I../../src/agent -I../../src
gcc -g -o SimuList.o -c ../../src/agent/SimuList.c -I../../src/agent -I../../src
gcc -g -o sernet_ut.o -c sernet_ut.c -I../../src/agent -I../../src

gcc -g -o sernet_ut sernet.o serclient.o SimuList.o sernet_ut.o -ll0001-0 -ll0002-0 -ll0003-0 -levent -levent_pthreads -lpthread -lm


# clean
rm -rf *.o

