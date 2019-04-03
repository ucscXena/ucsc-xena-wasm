CFLAGS=--std=gnu99 -O3
# gprof opts:
#CFLAGS=--std=gnu99 -O2 -pg
# also, bump test count up to get 30s of results, or so.
# Check gcov to see if works any better at O3.

bench_objects=radix.o fradix.o fradix16.o fradix16-64.o bench.o

bench: $(bench_objects)

CCALL=-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "setValue"]'
EXPORT=-s EXPORTED_FUNCTIONS='["_fradixSort16_64","_fradixSort16_64_init","_fradixSortL16_64","_malloc","_free"]'
MEMOPTS=-s ALLOW_MEMORY_GROWTH=1

sort.js: fradix16-64.o
	$(CC) $(CFLAGS) -o $@ $(CCALL) $(EXPORT) $(MEMOPTS) $^

bench.html: bench
	cp bench bench.bc
	$(CC) $(CFLAGS) bench.bc $(MEMOPTS) -o bench.html

clean:
	rm -f bench $(bench_objects)
