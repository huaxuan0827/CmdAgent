#file start

#CC = gcc
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip
SRCS := $(wildcard *.c) 
SRCS += 
OBJS := $(patsubst %.c,%.o, $(SRCS))
MAJOR := 1
MINOR := 0
VERSION := $(MAJOR).$(MINOR)
NAME := l0006-0
LIBNAME := lib$(NAME).so
SONAME := lib$(NAME).so.$(MAJOR)
LIBFULLNAME := lib$(NAME).so.$(VERSION)
#LIBFULLNAME := lib$(NAME).a

# for debug
#CFLAGS+=$(CFLAG) -Wall -O0 -g -ggdb
# for release
CFLAGS+=$(CFLAG) -Wall -O3 -fPIC -I../

#CFLAGS+=-I./ -I./include
#LIBS+=-lm -lpthread -lrt -lanl -lcurl
LIBS+=-lpthread -lm -L../../bin -ll0001-0 -ll0002-0 -ll0003-0


.PHONY: all clean 

$(LIBFULLNAME): $(OBJS)
	@$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(LIBS) $(CFLAGS)
	ln -sf $@ $(LIBNAME)
	ln -sf $@ $(SONAME)
	@echo [LK] $^ $(CFLAGS)
	mv $(LIBFULLNAME) ../../bin
	mv $(LIBNAME) ../../bin
	mv $(SONAME) ../../bin
	rm -f $(OBJS)
	
.c.o:
	@$(CC) -o $@ -c $(CFLAGS) $^
	@echo [CC] $^

clean:
	rm -f $(OBJS) $(LIBFULLNAME) $(LIBNAME) $(SONAME)
#file end
