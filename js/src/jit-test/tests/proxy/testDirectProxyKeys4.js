load(libdir + "asserts.js");

// Throw a TypeError if the trap reports the same property twice
assertThrowsInstanceOf(function () {
    Object.keys(new Proxy({}, {
        keys: function (target) {
            return [ 'foo', 'foo' ];
        }
    }));
}, TypeError);
