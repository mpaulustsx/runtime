[   ["assertEqual",	["#create runs and receives its arguments", {
        private _o = createHashMapObject [[["value", 0], ["#create", { _self set ["value", _this] }]], 10];
        _o get "value"
    } ], 10],
    ["assertEqual",	["the object is what the expression yields, not the constructor's result", {
        private _o = createHashMapObject [[["#create", { 42 }], ["marker", "here"]]];
        _o get "marker"
    } ], "here"],
    ["assertEqual",	["a prototype without #create still builds", {
        private _o = createHashMapObject [[["value", 7]]];
        _o get "value"
    } ], 7],
    ["assertEqual",	["a method sees the object as _self and its arguments as _this", {
        private _o = createHashMapObject [[["value", 1], ["bump", { _self set ["value", (_self get "value") + _this]; _self get "value" }]]];
        _o call ["bump", 4]
    } ], 5],
    ["assertIsNil",	["calling a name that is not a method yields nil", {
        private _o = createHashMapObject [[["value", 1]]];
        _o call ["nosuchmethod"]
    } ] ],
    ["assertEqual",	["#type with no #base anywhere is kept exactly as declared", {
        private _o = createHashMapObject [[["#type", "Counter"]]];
        _o get "#type"
    } ], "Counter"],
    ["assertEqual",	["a derived object inherits what it does not declare", {
        private _base = [["greet", { "hello" }], ["value", 1]];
        private _o = createHashMapObject [[["#base", _base], ["value", 2]]];
        _o call ["greet"]
    } ], "hello"],
    ["assertEqual",	["and overrides what it does declare", {
        private _base = [["greet", { "hello" }], ["value", 1]];
        private _o = createHashMapObject [[["#base", _base], ["value", 2]]];
        _o get "value"
    } ], 2],
    ["assertEqual",	["a hashmap prototype works as well as a pair array", {
        private _proto = createHashMapFromArray [["value", 3]];
        private _o = createHashMapObject [_proto];
        _o get "value"
    } ], 3],
    ["assertEqual",	["only the base declares #create, so only it runs for the derived object", {
        private _base = [["#create", { _self set ["value", _this * 2] }]];
        private _o = createHashMapObject [[["#base", _base]], 21];
        _o get "value"
    } ], 42],
    ["assertEqual",	["Constructors merge and run in sequence, base first, per the reference's own worked example", {
        private _log = [];
        private _animal = [["#type", "IAnimal"], ["#create", { (_this select 0) pushBack "Animal Init" }]];
        private _pig = [["#base", _animal], ["#type", "Pig"], ["#create", { (_this select 0) pushBack "Pig Init" }]];
        private _smolPig = [["#base", _pig], ["#type", "SmolPig"], ["#create", { (_this select 0) pushBack "SmolPig Init" }]];
        createHashMapObject [_smolPig, [_log]];
        _log
    } ], ["Animal Init", "Pig Init", "SmolPig Init"]],
    ["assertEqual",	["on inheritance #type is merged into an Array, base first", {
        private _animal = [["#type", "IAnimal"]];
        private _pig = [["#base", _animal], ["#type", "Pig"]];
        (createHashMapObject [_pig]) get "#type"
    } ], ["IAnimal", "Pig"]],
    ["assertTrue",	["the reference's own type-check example: in finds an inherited #type", {
        private _animal = [["#type", "IAnimal"], ["FurType", { "None" }]];
        private _pig = [["#base", _animal], ["#type", "Pig"], ["FurType", { "Bristles" }]];
        private _instance = createHashMapObject [_pig];
        "IAnimal" in (_instance get "#type")
    } ] ],
    ["assertEqual",	["and the derived method overrides the base one", {
        private _animal = [["#type", "IAnimal"], ["FurType", { "None" }]];
        private _pig = [["#base", _animal], ["#type", "Pig"], ["FurType", { "Bristles" }]];
        (createHashMapObject [_pig]) call ["FurType"]
    } ], "Bristles"],
    ["assertEqual",	["#str customises what str() returns for the object", {
        private _o = createHashMapObject [[["#str", { "My HashMap Object" }]]];
        str _o
    } ], "My HashMap Object"],
    ["assertEqual",	["without #str, str() falls back to the plain hashmap rendering", {
        str (createHashMapObject [[["value", 1]]])
    } ], "[[""value"",1]]"],
    ["assertEqual",	["#clone runs on a copy, base first, and does not disturb the original", {
        private _log = [];
        private _animal = [["#clone", { (_self get "log") pushBack "Animal Copied" }]];
        private _pig = [["#base", _animal], ["#clone", { (_self get "log") pushBack "Pig Copied" }]];
        private _o = createHashMapObject [_pig];
        _o set ["log", _log];
        private _copy = +_o;
        [_o get "log", _copy get "log"]
    } ], [[], ["Animal Copied", "Pig Copied"]]],
    ["assertException",	["noCopy makes + a runtime error rather than a silent copy", {
        private _o = createHashMapObject [[["#flags", ["noCopy"]]]];
        +_o
    } ]],
    ["assertTrue",	["sealed still allows editing a key the object already has", {
        private _o = createHashMapObject [[["#flags", ["sealed"]], ["value", 1]]];
        _o set ["value", 2];
        (_o get "value") == 2
    } ] ],
    ["assertTrue",	["sealed refuses to add a key that was not declared", {
        private _o = createHashMapObject [[["#flags", ["sealed"]], ["value", 1]]];
        _o set ["extra", 1];
        !("extra" in _o)
    } ] ],
    ["assertTrue",	["sealed refuses to remove a key at all", {
        private _o = createHashMapObject [[["#flags", ["sealed"]], ["value", 1]]];
        _o deleteAt "value";
        "value" in _o
    } ] ],
    ["assertTrue",	["flags are case-insensitive", {
        private _o = createHashMapObject [[["#flags", ["Sealed"]], ["value", 1]]];
        _o set ["extra", 1];
        !("extra" in _o)
    } ] ]
]
