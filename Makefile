CFLAGS=--std=gnu99 -O3
# gprof opts:
#CFLAGS=--std=gnu99 -O2 -pg
# also, bump test count up to get 30s of results, or so.
# Check gcov to see if works any better at O3.

.PHONY: all

TARGETS=radix fradix fradix16 fradix16-64

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

radix: radix.c

fradix: fradix.c

fradix16: fradix16.c

fradix16-64: fradix16-64.c

