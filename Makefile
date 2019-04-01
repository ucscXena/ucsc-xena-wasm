CFLAGS=--std=gnu99 -O3

.PHONY: all

all: radix fradix fradix16

radix: radix.c

fradix: fradix.c

fradix16: fradix16.c

