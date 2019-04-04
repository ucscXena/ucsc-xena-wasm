'use strict';

function toWASM(arr) {
	var arrWASM = Module._malloc(arr.length * arr.BYTES_PER_ELEMENT),
		u8 = new Uint8Array(arr.buffer);
	Module.HEAPU8.set(u8, arrWASM);
	return arrWASM;
}

var pointerSize = 4;
function marshalList(arrays) {
	var arrWASM = arrays.map(toWASM),
		list = Module._malloc(arrWASM.length * pointerSize);

	arrWASM.forEach(function(a, i) {
		Module.setValue(list + i * pointerSize, a, '*');

	});
	return list;
}

function freeList(list, n) {
	var a;
	for (var i = 0; i < n; ++ i) {
		a = Module.getValue(list + i * pointerSize, '*');
		Module._free(a);
	}
	Module._free(list);
}

// XXX Note that if we return a view of memory over the indiciesW result, we
// can't free it.  We need to allocate the indicies once, then free/allocate if
// the sample count changes.  Or, we have to copy the indicies out before
// freeing. That's what slice() does.
//
// XXX important safety tip: using ALLOW_MEMORY_GROWTH means
// a malloc call can invalidate views, because it grows by
// allocating a new, larger memory space. Perhaps we could
// use a proxy object that recreates views if they become
// obsolete. Or, live with the copy overhead on each call.
// XXX See also SPLIT_MEMORY, which may resolve this problem by
// using a different growth mechanism.

Module.fradixSortL16_64 = function(input, indicies) {
	var list = marshalList(input),
		indiciesW = toWASM(indicies),
		N = indicies.length;

	Module.ccall('fradixSortL16_64', null,
		// list, listM, N, indicies
		['number', 'number', 'number', 'number'],
		[list, input.length, N, indiciesW]);

	var r = new Uint32Array(Module.HEAPU8.buffer.slice(indiciesW, indiciesW + 4 * N));
	freeList(list);
	Module._free(indiciesW);
	return r;
}

Module.fradixSort16_64_init = function() {
	Module.ccall('fradixSort16_64_init', null, [], []);
}
