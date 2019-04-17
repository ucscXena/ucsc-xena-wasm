#!/usr/bin/env node

var fs = require('fs');
require('./polyfill');

function assocIn(x, path, v) {
	var o = x;
	for (var i = 0; i < path.length - 1; ++i) {
		if (!o.hasOwnProperty(path[i])) {
			o[path[i]] = {};
		}
		o = o[path[i]];
	}
	o[path[path.length - 1]] = parseInt(v, 10);
	return x;
}

function run(output, files) {
	var lines = files.map(file => fs.readFileSync(file).toString()
		.replace(/\n*$/, '').split(/\n/)
		.map(l => l.split(/\t/))).flat();

	var json = lines.reduce((acc, line) => {
		var v = line[line.length - 1],
			path = line.slice(0, line.length - 1);
		return assocIn(acc, path, v);
	}, {});

	var txt = `Object.assign(Module, ${JSON.stringify(json)});`

	fs.writeFileSync(output, txt);
}

if (process.argv.length < 4) {
	console.error(`${process.argv[1]} <output> <input>...`);
	process.exit(1);
}

var [output, ...files] = process.argv.slice(2);
run(output, files);
