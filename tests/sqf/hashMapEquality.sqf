/*
The engine's comparison operators take Number, Side, String, Object, Group,
Structured Text, Config and the other handle types; a HashMap operand is a
type error there, which is why BI ticket T167311 had to ask for a reference
comparison at all - and why isEqualRef exists today.

So == and != are deliberately not registered for HashMap here. Answering a
question the game refuses is the one thing a test runner must never do: a
mission built on hashmap-as-object-identity would compare its objects with ==
throughout, pass its entire suite, and then throw on a real server the first
time that line ran. Failing here instead is the whole point.

isEqualTo (content) and isEqualRef (instance) cover every honest question
between them, and both work in the game.
*/
[
    ["assertException",	["== refuses a hashmap operand", {
        private _h = createHashMap;
        _h == _h
    } ]],
    ["assertException",	["!= refuses a hashmap operand, the same way", {
        private _a = createHashMap;
        private _b = createHashMap;
        _a != _b
    } ]],
    ["assertException",	["aliasing the same hashmap does not make == legal", {
        private _h = createHashMap;
        private _alias = _h;
        _h == _alias
    } ]],
    ["assertTrue",	["isEqualRef answers the identity question == was reached for", {
        private _h = createHashMap;
        private _alias = _h;
        private _other = createHashMap;
        (_h isEqualRef _alias) && !(_h isEqualRef _other)
    } ]],
    ["assertTrue",	["isEqualTo answers the content question, and still works", {
        private _a = createHashMapFromArray [["n", 1]];
        private _b = createHashMapFromArray [["n", 1]];
        (_a isEqualTo _b) && (_a isNotEqualTo (createHashMapFromArray [["n", 2]]))
    } ]],
    ["assertTrue",	["== is untouched for the types the engine does accept", {
        (1 == 1) && !("a" == "b") && (west == west)
    } ]]
]
