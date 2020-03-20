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
NAME := l0001-0
LIBNAME := lib$(NAME).so
SONAME := lib$(NAME).so.$(MAJOR)
LIBFULLNAME := lib$(NAME).so.$(VERSION)

# for debug
#CFLAGS+=$(CFLAG) -Wall -O0 -g -ggdb
# for release
CFLAGS+=$(CFLAG) -Wall -O3 -fPIC

CFLAGS+=-I./ -I./include
#LIBS+=-lm -lpthread -lrt -lanl -lcurl
LIBS+=-lpthread -lm

.PHONY: all clean 

$(LIBFULLNAME): $(OBJS)
	@$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(LIBS)
	ln -sf $@ $(LIBNAME)
	ln -sf $@ $(SONAME)
	mv $(LIBNAME) ../../bin
	mv $(SONAME) ../../bin
	mv $(LIBFULLNAME) ../../bin
	@echo [LK] $^ $(CFLAGS)
	rm -f $(OBJS)
	
.c.o:
	@$(CC) -o $@ -c $(CFLAGS) $^
	@echo [CC] $^

clean:
	rm -f $(OBJS) $(LIBFULLNAME) $(LIBNAME) $(SONAME)
#file end
