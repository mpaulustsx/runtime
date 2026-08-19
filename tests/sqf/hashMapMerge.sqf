[   ["assertEqual",	["merge hashMap2 adds a key hashMap1 does not hold", {
        private _t = createHashMapFromArray [["a", 1]];
        _t merge (createHashMapFromArray [["b", 2]]);
        _t get "b"
    } ], 2],
    ["assertEqual",	["merge hashMap2 without an array never overwrites an existing key", {
        private _t = createHashMapFromArray [["a", 1]];
        _t merge (createHashMapFromArray [["a", 99]]);
        _t get "a"
    } ], 1],
    ["assertEqual",	["the doc's own worked example, plain merge", {
        private _hashmap1 = ["cow", "cat", "chicken"] createHashMapFromArray [100, 200, 200];
        private _hashmap2 = ["cow", "cat", "chicken", "camel"] createHashMapFromArray [150, 300, 400, 800];
        _hashmap1 merge _hashmap2;
        [_hashmap1 get "cow", _hashmap1 get "cat", _hashmap1 get "chicken", _hashmap1 get "camel"]
    } ], [100, 200, 200, 800]],
    ["assertEqual",	["without overwriteExisting an existing key is left alone", {
        private _t = createHashMapFromArray [["a", 1]];
        _t merge [createHashMapFromArray [["a", 99]], false];
        _t get "a"
    } ], 1],
    ["assertEqual",	["with overwriteExisting it is replaced", {
        private _t = createHashMapFromArray [["a", 1]];
        _t merge [createHashMapFromArray [["a", 99]], true];
        _t get "a"
    } ], 99],
    ["assertEqual",	["overwriteExisting defaults to leaving the target alone", {
        private _t = createHashMapFromArray [["a", 1]];
        _t merge [createHashMapFromArray [["a", 99]]];
        _t get "a"
    } ], 1],
    ["assertEqual",	["the doc's own worked example, overwriteExisting true", {
        private _hashmap1 = ["cow", "cat", "chicken"] createHashMapFromArray [100, 200, 200];
        private _hashmap2 = ["cow", "cat", "chicken", "camel"] createHashMapFromArray [150, 300, 400, 800];
        _hashmap1 merge [_hashmap2, true];
        [_hashmap1 get "cow", _hashmap1 get "cat", _hashmap1 get "chicken", _hashmap1 get "camel"]
    } ], [150, 300, 400, 800]],
    ["assertEqual",	["the doc's own worked example, overwriteExisting false", {
        private _hashmap1 = ["cow", "cat", "chicken"] createHashMapFromArray [100, 200, 200];
        private _hashmap2 = ["cow", "cat", "chicken", "camel"] createHashMapFromArray [150, 300, 400, 800];
        _hashmap1 merge [_hashmap2, false];
        [_hashmap1 get "cow", _hashmap1 get "cat", _hashmap1 get "chicken", _hashmap1 get "camel"]
    } ], [100, 200, 200, 800]],
    ["assertEqual",	["values returns one entry per key", {
        private _t = createHashMapFromArray [["a", 1], ["b", 2]];
        count values _t
    } ], 2],
    ["assertEqual",	["values of an empty hashmap is empty", {
        count values createHashMap
    } ], 0]
]
