[   ["assertEqual",	["an existing key returns its own value and the code never runs", {
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefaultCall ["a", { 99 }]
    } ], 1],
    ["assertTrue",	["the default code runs only when the key is missing", {
        private _ran = false;
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefaultCall ["a", { _ran = true; 99 }];
        !_ran
    } ] ],
    ["assertEqual",	["a missing key runs the code and returns what it produced", {
        (createHashMapFromArray [["a", 1]]) getOrDefaultCall ["b", { 26 }]
    } ], 26],
    ["assertEqual",	["the code receives the missing key as _this", {
        (createHashMapFromArray [["a", 1]]) getOrDefaultCall ["z", { _this }]
    } ], "z"],
    ["assertTrue",	["without setDefault a missing key is not stored", {
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefaultCall ["b", { 26 }];
        !("b" in _t)
    } ] ],
    ["assertTrue",	["with setDefault a missing key is stored", {
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefaultCall ["b", { 26 }, true];
        "b" in _t
    } ] ],
    ["assertEqual",	["what setDefault stores is what the code produced, not re-run", {
        private _t = createHashMapFromArray [["a", 1]];
        _t getOrDefaultCall ["b", { 26 }, true];
        _t get "b"
    } ], 26]
]
