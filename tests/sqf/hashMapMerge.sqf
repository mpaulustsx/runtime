[   ["assertEqual",	["merge adds keys the target does not hold", {
        private _t = createHashMapFromArray [["a", 1]];
        _t merge [createHashMapFromArray [["b", 2]], false];
        _t get "b"
    } ], 2],
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
    ["assertEqual",	["the source may be an array of pairs", {
        private _t = createHashMapFromArray [["a", 1]];
        _t merge [[["b", 2]], false];
        _t get "b"
    } ], 2],
    ["assertEqual",	["values returns one entry per key", {
        private _t = createHashMapFromArray [["a", 1], ["b", 2]];
        count values _t
    } ], 2],
    ["assertEqual",	["values of an empty hashmap is empty", {
        count values createHashMap
    } ], 0]
]
