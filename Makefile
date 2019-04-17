CFLAGS=--std=gnu99 -O3
# gprof opts:
#CFLAGS=--std=gnu99 -O2 -pg
# also, bump test count up to get 30s of results, or so.
# Check gcov to see if works any better at O3.

bench_objects=radix.o fradix.o fradix16.o fradix16-64.o bench.o

bench: $(bench_objects)

test_stats_objects=stats.o test_stats.o fradix16.o

test_stats: $(test_stats_objects)

RTEXPORT=-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "setValue", "getValue"]'
EXPORT=-s EXPORTED_FUNCTIONS='["_fradixSort16_64","_fradixSort16_64_init","_fradixSortL16_64","_fradixSort16_init","_malloc","_free","_faminmax","_faminmax_init","_fameanmedian_init","_fameanmedian","_get_color_linear","_get_color_log2","_region_color_linear_test","_get_get_color_linear","_get_get_color_log2","_draw_subcolumn","_tally_domains","_calloc"]'
SORTFLAGS=-s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 --pre-js wrappers.js --pre-js wasm_struct.js

%.idl.c %.h: %.idl
	./idl.js $< $@ $*.h

%.idl.c.js %.idl.c.wasm: %.idl.c
	emcc -o $@ $<

%.idl.tsv: %.idl.c.js
	node $< > $@

wasm_struct.js: heatmap_struct.idl.tsv color_scales_struct.idl.tsv
	./idlToJSON.js $@ $^

METHODS=fradix16-64.o fradix16.o stats.o color_scales.o heatmap.o

xena.js: $(METHODS) wrappers.js wasm_struct.js
	$(CC) $(CFLAGS) -o $@ $(RTEXPORT) $(EXPORT) $(SORTFLAGS) $(METHODS)

bench.html: bench
	cp bench bench.bc
	$(CC) $(CFLAGS) bench.bc $(MEMOPTS) -o bench.html

clean:
	rm -f *.map *.wast bench heatmap_struct.h color_scales_struct.h  *.o *.wasm *.bc *.idl.c *.idl.tsv *.idl.c.js wasm_struct.js xena.js xena.wasm bench bench.html $(test_stats_objects) test_stats
