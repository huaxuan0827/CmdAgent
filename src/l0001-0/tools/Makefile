#file start

#CC = gcc
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
SRCS := $(wildcard *.c) $(wildcard misc/*.c)  $(wildcard algorithm/*.c) $(wildcard err/*.c) $(wildcard filter/*.c) $(wildcard serialize/*.c) 
SRCS += 
OBJS := $(patsubst %.c,%.o, $(SRCS))
ReleaseFile = test

# for release
CFLAGS+=$(CFLAG) -Wall -O3 -mfloat-abi=hard

CFLAGS+=-I./ -I./include -L../
LIBS+=-ll0001-0

.PHONY: clean

all: $(ReleaseFile)

$(ReleaseFile): $(OBJS)
	@$(CC) $(CFLAGS) $^ $(LIBS) -o $@
	@echo [LK] $^ $(CFLAGS)
	
.c.o:
	@$(CC) -o $@ -c $(CFLAGS) $^
	@echo [CC] $^

clean:
	rm -f $(OBJS) $(ReleaseFile)
#file end
