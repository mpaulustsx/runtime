/*
isEqualRef / isNotEqualRef ask whether two values are the same underlying
instance. isEqualTo asks whether they hold equal content right now. The
reference documents the two side by side because they genuinely differ for
the reference types: two separately built but structurally identical arrays
or hashmaps are isEqualTo and are not isEqualRef.

That distinction is why membership - find, in, pushBackUnique - is left
comparing by content: it is what the engine does, and a test runner that
answered by instance instead would pass code the real game breaks on.
Anything wanting identity should ask for it explicitly, which is what these
two commands are for.
*/
[
    ["assertTrue",	["a value is the same instance as itself", {
        private _h = createHashMap;
        private _alias = _h;
        _h isEqualRef _alias
    } ] ],
    ["assertFalse",	["two hashmaps holding equal content are not the same instance", {
        private _a = createHashMapFromArray [["n", 1]];
        private _b = createHashMapFromArray [["n", 1]];
        _a isEqualRef _b
    } ] ],
    ["assertTrue",	["...while isEqualTo still says their content matches", {
        private _a = createHashMapFromArray [["n", 1]];
        private _b = createHashMapFromArray [["n", 1]];
        _a isEqualTo _b
    } ] ],
    ["assertTrue",	["the same split applies to arrays", {
        private _x = [1, [2, [3]]];
        private _y = [1, [2, [3]]];
        (_x isEqualTo _y) && !(_x isEqualRef _y)
    } ] ],
    ["assertTrue",	["a copy is equal in content but is a different instance", {
        private _x = [1, 2, 3];
        private _copy = +_x;
        (_x isEqualTo _copy) && !(_x isEqualRef _copy)
    } ] ],
    ["assertFalse",	["values of different types are never the same instance", {
        (createHashMap) isEqualRef []
    } ] ],
    ["assertTrue",	["a value type has no instance to speak of, so content decides", {
        (1 isEqualRef 1) && ("a" isEqualRef "a") && (true isEqualRef true)
    } ] ],
    ["assertTrue",	["isNotEqualRef is the exact inverse", {
        private _a = createHashMapFromArray [["n", 1]];
        private _b = createHashMapFromArray [["n", 1]];
        (_a isNotEqualRef _b) && !(_a isNotEqualRef _a)
    } ] ]
]
