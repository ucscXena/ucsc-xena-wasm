CFLAGS=--std=gnu99 -O3
# gprof opts:
#CFLAGS=--std=gnu99 -O2 -pg
# also, bump test count up to get 30s of results, or so.
# Check gcov to see if works any better at O3.

bench_objects=radix.o fradix.o fradix16.o fradix16-64.o bench.o

bench: $(bench_objects)

RTEXPORT=-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "setValue", "getValue"]'
EXPORT=-s EXPORTED_FUNCTIONS='["_fradixSort16_64","_fradixSort16_64_init","_fradixSortL16_64","_malloc","_free","_faminmax","_faminmax_init"]'
SORTFLAGS=-s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 --pre-js wrappers.js

METHODS=fradix16-64.o stats.o

xena.js: $(METHODS) wrappers.js
	$(CC) $(CFLAGS) -o $@ $(RTEXPORT) $(EXPORT) $(SORTFLAGS) $(METHODS)

bench.html: bench
	cp bench bench.bc
	$(CC) $(CFLAGS) bench.bc $(MEMOPTS) -o bench.html

clean:
	rm -f bench $(bench_objects) xena.js xena.wasm bench bench.bc bench.html
