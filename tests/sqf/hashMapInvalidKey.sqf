/*
A HashMap is a mutable reference type, not a hashable value, and the real
engine rejects one used as a HashMap key rather than silently accepting it.
This VM's own std::unordered_map<value, value> would otherwise hash and store
one without complaint (d_hashmap::hash() is cycle-safe and happy to), so left
unchecked, code that keys a cache by a live hashmap-storage object passes
every run here and only breaks on a real server. Every key-consuming op is
covered, not just set/get, since a lookup with an invalid key type is the same
kind of programmer error as a write with one.
*/
[
    ["assertException",	["set refuses a hashmap key", {
        (createHashMap) set [createHashMap, 1]
    } ]],
    ["assertException",	["get refuses a hashmap key", {
        (createHashMap) get (createHashMap)
    } ]],
    ["assertException",	["getOrDefault refuses a hashmap key", {
        (createHashMap) getOrDefault [createHashMap, 1]
    } ]],
    ["assertException",	["getOrDefaultCall refuses a hashmap key", {
        (createHashMap) getOrDefaultCall [createHashMap, {1}]
    } ]],
    ["assertException",	["deleteAt refuses a hashmap key", {
        (createHashMap) deleteAt (createHashMap)
    } ]],
    ["assertException",	["in refuses a hashmap key on the left", {
        (createHashMap) in (createHashMap)
    } ]],
    ["assertException",	["createHashMapFromArray (pair-array form) refuses a hashmap key", {
        createHashMapFromArray [[createHashMap, 1]]
    } ]],
    ["assertException",	["createHashMapFromArray (keys/values form) refuses a hashmap key", {
        [createHashMap] createHashMapFromArray [1]
    } ]],
    ["assertTrue",	["every other key type still works, unaffected by the guard", {
        private _h = createHashMap;
        _h set ["a string", 1];
        _h set [2, "a scalar"];
        _h set [true, "a bool"];
        ("a string" in _h) && (2 in _h) && (true in _h)
    } ] ]
]
