CFLAGS=--std=gnu99 -O3
# gprof opts:
#CFLAGS=--std=gnu99 -O2 -pg
# also, bump test count up to get 30s of results, or so.
# Check gcov to see if works any better at O3.

#
# generate dependencies
#
SRCS= baos.c bench.c color_scales.c fradix16-64.c fradix16.c fradix.c heatmap.c \
	hfc.c htfc.c hfcz.c htfcz.c huffman.c queue.c radix.c roaring.c roaring_test.c stats.c array.c \
	bytes.c test_baos.c test_hfc.c test_htfc.c test_huffman.c test_queue.c test_stats.c test.c

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

test: test.o test_baos.o baos.o test_queue.o queue.o test_stats.o stats.o fradix16.o test_huffman.o huffman.o test_htfc.o htfc.o test_hfc.o hfc.o bytes.o array.o
		$(CC) $(CFLAGS) -o $@ $^ -lcheck -lm -lrt -lpthread -lsubunit

RTEXPORT=-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "setValue", "getValue"]'
EXPORT=-s EXPORTED_FUNCTIONS='["_fradixSort16_64","_fradixSort16_64_init","_fradixSortL16_64","_fradixSort16_init","_malloc","_free","_faminmax","_faminmax_init","_fameanmedian_init","_fameanmedian","_get_color_linear","_get_color_log2","_region_color_linear_test","_draw_subcolumn","_tally_domains","_test_scale_method","_hfc_search","_hfc_set","_hfc_merge","_hfc_filter","_hfc_lookup"]'
SORTFLAGS=-s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 --pre-js wrappers.js --pre-js wasm_struct.js

IDL := $(wildcard *.idl)
IDL_DERIVED := $(IDL:.idl=.js) $(IDL:.idl=.c) $(IDL:.idl=.h) $(IDL:.idl=.wasm) $(IDL:.idl=.probe)

%.c %.h: %.idl
	./idl.js $*.idl $*.c $*.h

%.js %.wasm: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.probe: %.js
	node $< > $@

wasm_struct.js: heatmap_struct.probe color_scales_struct.probe htfc_struct.probe hfc_struct.probe
	./idlToJSON.js $@ $^

htfcz: htfcz.o htfc.o baos.o huffman.o queue.o
	$(CC) $(CFLAGS) -o $@ $^

METHODS=fradix16-64.o fradix16.o stats.o color_scales.o heatmap.o huffman.o baos.o bytes.o array.o queue.o htfc.o hfc.o

hfcz: hfcz.o hfc.o baos.o huffman.o queue.o bytes.o array.o
	$(CC) $(CFLAGS) -o $@ $^


xena.js: $(METHODS) wrappers.js wasm_struct.js
	$(CC) $(CFLAGS) -o $@ $(RTEXPORT) $(EXPORT) $(SORTFLAGS) $(METHODS)

bench.html: bench
	cp bench bench.bc
	$(CC) $(CFLAGS) bench.bc $(MEMOPTS) -o bench.html

clean:
	rm -f *.map *.wast bench heatmap_struct.h color_scales_struct.h  *.o *.wasm *.bc hfcz htfcz wasm_struct.js xena.js bench bench.html test $(IDL_DERIVED)


# include dependencies
include $(wildcard $(DEPFILES))
