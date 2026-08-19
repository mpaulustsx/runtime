[   ["assertEqual",	["keys createHashMapFromArray values pairs them up in order", {
        private _t = ["cow", "cat"] createHashMapFromArray [100, 200];
        [_t get "cow", _t get "cat"]
    } ], [100, 200]],
    ["assertEqual",	["an empty pair of arrays makes an empty hashmap", {
        count ([] createHashMapFromArray [])
    } ], 0],
    ["assertEqual",	["the single-array form still works alongside the two-array form", {
        count (createHashMapFromArray [["a", 1], ["b", 2]])
    } ], 2],
    ["assertTrue",	["the reference's own example: no values at all fills every key with nil", {
        private _t = [1, 2, 3, 4] createHashMapFromArray [];
        (isNil { _t get 1 }) && (isNil { _t get 2 }) && (isNil { _t get 3 }) && (isNil { _t get 4 })
    } ] ],
    ["assertTrue",	["a key genuinely missing its value is still a member, holding nil", {
        private _t = [1, 2, 3, 4] createHashMapFromArray [];
        4 in _t
    } ] ],
    ["assertEqual",	["the reference's own example: extra values past the last key are dropped, not paired up on their own", {
        private _t = [1, 2, 3] createHashMapFromArray ["one", "two", "three", "four"];
        [count _t, _t get 1, _t get 2, _t get 3]
    } ], [3, "one", "two", "three"]],
    ["assertFalse",	["so a value with no key of its own never becomes a member", {
        private _t = [1, 2, 3] createHashMapFromArray ["one", "two", "three", "four"];
        "four" in _t
    } ] ]
]
