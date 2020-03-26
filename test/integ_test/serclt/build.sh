#!/bin/bash

gcc ser_clt.c -o ser_clt -I../../../src/agent
gcc ser_clt2.c -o ser_clt2 -I../../../src/agent

rm -rf *.o

