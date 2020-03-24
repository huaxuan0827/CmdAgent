#!/bin/bash

# build devnet_ut.
gcc -g -o devnet.o -c ../../src/agent/devnet.c -I../../src/agent -I../../src
gcc -g -o devnet_ut.o -c devnet_ut.c -I../../src/agent -I../../src

gcc -g -o devnet_ut devnet.o devnet_ut.o -ll0001-0 -ll0002-0 -ll0003-0 -levent -lpthread -lm

# build other.


# clean
rm -rf *.o

