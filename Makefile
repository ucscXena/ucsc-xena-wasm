all: linux wasm

CFLAGS=--std=gnu99 -O3

CHECKFLAGS=-Icheck-build -Icheck-build/src

# gprof opts:
#CFLAGS=--std=gnu99 -O2 -pg
# also, bump test count up to get 30s of results, or so.
# Check gcov to see if works any better at O3.

#
# generate dependencies
#
SRCS= baos.c bench.c color_scales.c fradix16-64.c fradix16.c fradix.c heatmap.c \
	hfc.c htfc.c hfcz.c htfcz.c huffman.c queue.c radix.c roaring.c roaring_test.c stats.c array.c \
	bytes.c test_baos.c test_hfc.c test_htfc.c test_huffman.c test_queue.c test_stats.c test.c \
	test_sort.c

DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $(CHECKFLAGS) -c

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

#
# local cache of libcheck
#

.PHONY: install_check

install_check: | check-0.13.0

CHECK_RELEASE := 0.13.0
CHECK_TGZ := check-$(CHECK_RELEASE).tar.gz
CHECK_LD_PATH := check-build/src/.libs

check-0.13.0:
	wget https://github.com/libcheck/check/releases/download/$(CHECK_RELEASE)/$(CHECK_TGZ)
	tar zxf $(CHECK_TGZ)

check-build/Makefile: | ../check-$(CHECK_RELEASE)
	mkdir -p check-build
	cd check-build && ../../check-$(CHECK_RELEASE)/configure --enable-shared=false

$(CHECK_LD_PATH)/libcheck.a: | check-build/Makefile
	# clear MAKEOVERRIDES so the check build doesn't pick up
	# xena stuff, like VPATH.
	cd check-build && $(MAKE) MAKEOVERRIDES=

#
# idl rules
#

IDL := $(wildcard *.idl)
IDL_SRC := $(IDL:.idl=.c) $(IDL:.idl=.h)

%.c %.h: %.idl
	./idl.js $*.idl $*.c $*.h

%.js %.wasm: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.probe: %.js
	node $< > $@

#
# build linux and wasm targets
#

.PHONY: platforms linux wasm

platforms: linux wasm

linux: $(IDL_SRC) | build-linux
	$(MAKE) -C build-linux VPATH=.. -f $(CURDIR)/Makefile all-linux

wasm: $(IDL_SRC) | build-wasm
	emmake $(MAKE) -C build-wasm VPATH=.. -f $(CURDIR)/Makefile all-wasm

build-linux build-wasm:
	mkdir -p $@

.PHONY: all-linux all-wasm

all-linux: test hfcz

all-wasm: test.js xena.js

#
#
#

bench_objects=radix.o fradix.o fradix16.o fradix16-64.o bench.o

bench: $(bench_objects)

TESTOBJ=test.o test_baos.o baos.o test_queue.o queue.o test_stats.o stats.o fradix16.o test_huffman.o huffman.o test_htfc.o htfc.o test_hfc.o hfc.o bytes.o array.o test_sort.o fradix16-64.o

test: $(CHECK_LD_PATH)/libcheck.a $(TESTOBJ)
		LD_RUN_PATH=$(CURDIR)/$(CHECK_LD_PATH) $(CC) $(CFLAGS) -o $@ $^ -L$(CHECK_LD_PATH) -lcheck -lm -lrt -lpthread

test.js: $(CHECK_LD_PATH)/libcheck.a $(TESTOBJ)
	$(CC) $(CFLAGS) -o $@ $^ -L$(CHECK_LD_PATH) -lcheck -s ALLOW_MEMORY_GROWTH=1 --embed-file ../data

RTEXPORT=-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "setValue", "getValue", "UTF8ToString", "writeAsciiToMemory"]'
EXPORT=-s EXPORTED_FUNCTIONS='["_fradixSort16_64","_fradixSort16_64_init","_fradixSortL16_64","_fradixSort16_init","_malloc","_free","_faminmax","_faminmax_init","_fameanmedian_init","_fameanmedian","_get_color_linear","_get_color_log2","_region_color_linear_test","_draw_subcolumn","_tally_domains","_test_scale_method","_hfc_search","_hfc_set","_hfc_set_empty","_hfc_merge","_hfc_filter","_hfc_lookup","_hfc_length","_hfc_buff_length","_hfc_buff","_hfc_compress"]'
SORTFLAGS=-s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 --pre-js ../wrappers.js --pre-js wasm_struct.js

wasm_struct.js: heatmap_struct.probe color_scales_struct.probe htfc_struct.probe hfc_struct.probe fradix16-64_struct.probe bytes_struct.probe
	../idlToJSON.js $@ $^

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

.PHONY: clean

clean:
	rm -f build-linux/* build-wasm/* $(IDL_SRC) | true

clean-all:
	rm -rf build-linux build-wasm $(IDL_SRC)

# include dependencies
include $(wildcard $(DEPFILES))
