[   ["assertEqual",	["ascending sort of arrays compares element by element", {
        private _a = [[2, "b"], [1, "a"], [3, "c"]];
        _a sort true;
        _a
    } ], [[1, "a"], [2, "b"], [3, "c"]]],
    ["assertEqual",	["descending sort of arrays does too, and does not crash", {
        private _a = [[1, "a"], [3, "c"], [2, "b"]];
        _a sort false;
        _a
    } ], [[3, "c"], [2, "b"], [1, "a"]]],
    ["assertEqual",	["ties on the first element fall through to the second", {
        private _a = [[1, "z"], [1, "a"]];
        _a sort true;
        _a
    } ], [[1, "a"], [1, "z"]]],
    ["assertEqual",	["a single distinct array does not crash the comparator", {
        private _a = [[1, 2], [1, 2], [1, 2]];
        _a sort false;
        count _a
    } ], 3]
]
