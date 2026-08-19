[   ["assertEqual",	["keys createHashMapFromArray values pairs them up in order", {
        private _t = ["cow", "cat"] createHashMapFromArray [100, 200];
        [_t get "cow", _t get "cat"]
    } ], [100, 200]],
    ["assertEqual",	["an empty pair of arrays makes an empty hashmap", {
        count ([] createHashMapFromArray [])
    } ], 0],
    ["assertEqual",	["the single-array form still works alongside the two-array form", {
        count (createHashMapFromArray [["a", 1], ["b", 2]])
    } ], 2]
]
