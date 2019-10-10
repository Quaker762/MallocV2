CC=gcc
OBJDUMP=objdump
NM=nm
CFLAGS=-g -Wall -Wextra -Wpedantic -std=gnu99

OBJS := $(patsubst %.c, %.o, $(wildcard source/*.c))

all: debug

release: $(OBJS)
	$(CC) $(OBJS) -lm -lpthread -o alloc
	#$(NM) alloc

debug: $(OBJS)
	$(CC) $(OBJS) -lm -lpthread -o alloc
	#$(NM) alloc	

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f source/*.o
	rm -f alloc

.PHONY: all clean
