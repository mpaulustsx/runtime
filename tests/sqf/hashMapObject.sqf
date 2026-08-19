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
    ["assertEqual",	["#type is kept on the object", {
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
    ["assertEqual",	["an inherited #create runs for the derived object", {
        private _base = [["#create", { _self set ["value", _this * 2] }]];
        private _o = createHashMapObject [[["#base", _base]], 21];
        _o get "value"
    } ], 42],
    ["assertEqual",	["a hashmap prototype works as well as a pair array", {
        private _proto = createHashMapFromArray [["value", 3]];
        private _o = createHashMapObject [_proto];
        _o get "value"
    } ], 3]
]
