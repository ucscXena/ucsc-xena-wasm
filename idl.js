#!/usr/bin/env node

// pass an _idl.js file. Emit a js file with struct offsets, by first emitting
// and running a wasm program that spills the offsets.


var path = require('path');
var fs = require('fs');
require('./polyfill');

var mapObject = (obj = {}, fn) =>
	Object.keys(obj).map(k => fn(obj[k], k));

function structProbeCode(fields, name) {
	return [
	`\tprintf("struct\\t${name}\\tsize\\t%lu\\n", sizeof(struct ${name}));`,
	...(fields.map(([type, field, count]) =>
		`\tprintf("struct\\t${name}\\toffset\\t${field}\\t%lu\\n", (uintptr_t)&((struct ${name}*)0)->${field});`))];
}

function enumProbeCode(vals, name) {
	return vals.map((v, i) =>
		`\tprintf("enum\\t${name}\\t${v}\\t${i}\\n");`)
}

function probeCode(input, header) {
	return [...input.includes.map(i => `#include ${i}`),
	`#include "${header}"`,
	'#include <stdio.h>',
	'#include <inttypes.h>',
	'int main(int argc, char *argv[]) {',
	...mapObject(input.struct, structProbeCode).flat(),
	...mapObject(input.enum, enumProbeCode).flat(),
	'}'].join('\n') + '\n';
}


function structCode(fields, name) {
	return [
	`struct ${name} {`,
	...(fields.map(([type, field, count]) =>
		`\t${type} ${field}${count == null ? '' : `[${count}]`};`)),
		'};'];
}

function enumCode(vals, name) {
	return [`enum ${name} {${vals.join(', ')}};`]
}

function headerCode(input) {
	return [
		...mapObject(input.struct, structCode).flat(),
		...mapObject(input.enum, enumCode).flat()
	].join('\n') + '\n';
}

// Can we all the routines into a single executable? Is there a reason to?
function run(input, probe, header) {
	var file = path.resolve(input);


	var input = require(file);

	fs.writeFileSync(header, headerCode(input));

	fs.writeFileSync(probe, probeCode(input, header));
}

if (process.argv.length != 5) {
	console.error(`${process.argv[1]} <input> <probe-file> <header-file>`);
	process.exit(1);
}

var [input, probe, header] = process.argv.slice(2);
run(input, probe, header);
