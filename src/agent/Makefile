#file start

#CC = gcc
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip
SRCS := $(wildcard *.c) $(wildcard serialize/*.c) 
SRCS += 
OBJS := $(patsubst %.c,%.o, $(SRCS))
RELEASE := CmdAgent


# for debug
#CFLAGS+=$(CFLAG) -Wall -O0 -g -ggdb
# for release
CFLAGS+=$(CFLAG) -g -Wall -O0

CFLAGS+=-I./ -I../
LIBS+=-ll0001-0 -ll0002-0 -ll0003-0 -lpthread -lm -lrt -lanl -levent -levent_pthreads 


.PHONY: all clean 

$(RELEASE): $(OBJS)
	@$(CC) $(CFLAGS) $^ $(LIBS) -o $@
	@echo [LK] $^ $(CFLAGS)
#	scp a0001-0 devel@192.168.1.9:/home/devel/appl/
	
.c.o:
	@$(CC) -o $@ -c $(CFLAGS) $^
	@echo [CC] $^

clean:
	rm -f $(OBJS) $(RELEASE)
#file end
