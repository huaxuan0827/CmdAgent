#!/bin/bash

gcc -g -o devnet.o -c ../../agent/devnet.c -I../../agent -I../../
gcc -g -o devnet_test.o -c devnet_test.c -I../../agent -I../../

gcc -g -o devnet_test devnet.o devnet_test.o -ll0001-0 -ll0002-0 -ll0003-0 -levent -lpthread -lm

rm -rf *.o
