[   ["assertEqual",	["an existing key returns its own value, not the default", {
        (createHashMapFromArray [["a", 1]]) getOrDefault ["a", 99]
    } ], 1],
    ["assertEqual",	["a missing key returns the default", {
        (createHashMapFromArray [["a", 1]]) getOrDefault ["b", 99]
    } ], 99],
    ["assertTrue",	["without setDefault a missing key is not stored", {
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefault ["b", 99];
        !("b" in _t)
    } ] ],
    ["assertTrue",	["with setDefault a missing key is stored", {
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefault ["b", 99, true];
        "b" in _t
    } ] ],
    ["assertEqual",	["what setDefault stores is what was returned", {
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefault ["b", 99, true];
        _t get "b"
    } ], 99],
    ["assertEqual",	["setDefault does not disturb a key that already exists", {
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefault ["a", 99, true];
        _t get "a"
    } ], 1],
    ["assertEqual",	["a new array is created per the doc's own example", {
        private _hashmap = createHashMap;
        private _array = _hashmap getOrDefault ["key", [], true];
        [count _array, "key" in _hashmap]
    } ], [0, true]]
]
