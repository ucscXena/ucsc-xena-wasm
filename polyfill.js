'use strict';

// XXX doesn't implement depth, and doesn't scale.
if (!Array.prototype.flat) {
	Array.prototype.flat = function () {
		return Array.prototype.concat.apply([], this);
	}
}
