/*
Array membership - find/in/pushBackUnique, and the - / arrayIntersect
operators - asks "is this element the same value as mine". For a HashMap that
can only mean the same object: two distinct objects hold equal data all the
time (any two freshly constructed objects of the same class do, before their
distinguishing fields are set), and a structural answer there silently returns
the wrong index, reports a non-member as present, or drops a distinct object.

isEqualTo keeps comparing content - that is still a separate, useful question,
just not the one membership asks.
*/
[
    ["assertTrue",	["find locates each distinct object at its own index", {
        private _a = createHashMapFromArray [["n", 1]];
        private _b = createHashMapFromArray [["n", 1]];
        private _arr = [_a, _b];
        ((_arr find _a) == 0) && ((_arr find _b) == 1)
    } ] ],
    ["assertEqual",	["find reports a non-member as absent even when its content matches", {
        private _a = createHashMapFromArray [["n", 1]];
        private _other = createHashMapFromArray [["n", 1]];
        [_a] find _other
    } ], -1],
    ["assertTrue",	["in answers by identity, not by content", {
        private _a = createHashMapFromArray [["n", 1]];
        private _other = createHashMapFromArray [["n", 1]];
        (_a in [_a]) && !(_other in [_a])
    } ] ],
    ["assertEqual",	["pushBackUnique keeps two distinct objects that happen to hold equal data", {
        private _a = createHashMapFromArray [["n", 1]];
        private _b = createHashMapFromArray [["n", 1]];
        private _arr = [];
        _arr pushBackUnique _a;
        _arr pushBackUnique _b;
        count _arr
    } ], 2],
    ["assertEqual",	["pushBackUnique still refuses the very same object twice", {
        private _a = createHashMapFromArray [["n", 1]];
        private _arr = [];
        _arr pushBackUnique _a;
        _arr pushBackUnique _a;
        count _arr
    } ], 1],
    ["assertTrue",	["- removes the named object and leaves its equal-looking sibling", {
        private _a = createHashMapFromArray [["n", 1]];
        private _b = createHashMapFromArray [["n", 1]];
        private _remaining = [_a, _b] - [_a];
        (count _remaining == 1) && ((_remaining select 0) == _b)
    } ] ],
    ["assertTrue",	["isEqualTo still compares content, unlike membership", {
        private _a = createHashMapFromArray [["n", 1]];
        private _b = createHashMapFromArray [["n", 1]];
        (_a isEqualTo _b) && !(_a == _b)
    } ] ],
    ["assertTrue",	["membership of non-hashmap values is unchanged", {
        private _arr = ["a", 2, true];
        ((_arr find 2) == 1) && ("a" in _arr) && !("z" in _arr) && (([1,2,3] - [2]) isEqualTo [1,3])
    } ] ]
]
