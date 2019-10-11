CFLAGS=--std=gnu99 -O3
# gprof opts:
#CFLAGS=--std=gnu99 -O2 -pg
# also, bump test count up to get 30s of results, or so.
# Check gcov to see if works any better at O3.

#
# generate dependencies
#
SRCS= baos.c bench.c color_scales.c fradix16-64.c fradix16.c fradix.c heatmap.c \
	htfc.c htfcz.c huffman.c queue.c radix.c roaring.c roaring_test.c stats.c \
	test_baos.c test_htfc.c test_huffman.c test_queue.c test_stats.c

DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

#
#
#

bench_objects=radix.o fradix.o fradix16.o fradix16-64.o bench.o

bench: $(bench_objects)

test_stats_objects=stats.o test_stats.o fradix16.o

test_stats: $(test_stats_objects)

test_baos_objects=baos.o test_baos.o

test_baos: $(test_baos_objects)

test_htfc_objects=htfc.o huffman.o baos.o test_htfc.o

test_htfc: $(test_htfc_objects)

test_queue: queue.o test_queue.o

test_huffman: huffman.o queue.o baos.o test_huffman.o

RTEXPORT=-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "setValue", "getValue"]'
EXPORT=-s EXPORTED_FUNCTIONS='["_fradixSort16_64","_fradixSort16_64_init","_fradixSortL16_64","_fradixSort16_init","_malloc","_free","_faminmax","_faminmax_init","_fameanmedian_init","_fameanmedian","_get_color_linear","_get_color_log2","_region_color_linear_test","_draw_subcolumn","_tally_domains","_test_scale_method","_htfc_search_store","_htfc_store"]'
SORTFLAGS=-s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 --pre-js wrappers.js --pre-js wasm_struct.js

IDL := $(wildcard *.idl)
IDL_DERIVED := $(IDL:.idl=.js) $(IDL:.idl=.c) $(IDL:.idl=.h) $(IDL:.idl=.wasm) $(IDL:.idl=.probe)

foo:
	echo $(IDL)
	echo $(IDL_DERIVED)

%.c %.h: %.idl
	./idl.js $*.idl $*.c $*.h

%.js %.wasm: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.probe: %.js
	node $< > $@

wasm_struct.js: heatmap_struct.probe color_scales_struct.probe htfc_struct.probe
	./idlToJSON.js $@ $^

color_scales.o: color_scales.h color_scales_struct.h

heatmap.o: heatmap_struct.h color_scales.h color_scales_struct.h

htfc.o: htfc.h htfc_struct.h baos.h huffman.h

METHODS=fradix16-64.o fradix16.o stats.o color_scales.o heatmap.o huffman.o baos.o htfc.o

xena.js: $(METHODS) wrappers.js wasm_struct.js
	$(CC) $(CFLAGS) -o $@ $(RTEXPORT) $(EXPORT) $(SORTFLAGS) $(METHODS)

bench.html: bench
	cp bench bench.bc
	$(CC) $(CFLAGS) bench.bc $(MEMOPTS) -o bench.html

clean:
	rm -f *.map *.wast bench heatmap_struct.h color_scales_struct.h  *.o *.wasm *.bc wasm_struct.js xena.js bench bench.html test_stats test_htfc test_baos $(IDL_DERIVED)


# include dependencies
include $(wildcard $(DEPFILES))
