#include "ops_hashmap.h"
#include "../runtime/value.h"
#include "../runtime/logging.h"
#include "../runtime/runtime.h"
#include "../runtime/sqfop.h"
#include "../runtime/util.h"

#include "../runtime/d_string.h"
#include "../runtime/d_scalar.h"
#include "../runtime/d_boolean.h"
#include "../runtime/d_array.h"
#include "../runtime/d_code.h"
#include "../runtime/instruction_set.h"

#include <array>



namespace err = logmessage::runtime;
using namespace sqf::runtime;
using namespace sqf::types;

namespace
{
    // ---------------------------------------------------------------------
    //  Hashmap objects
    //
    //  A hashmap object is an ordinary hashmap whose values may be code, plus
    //  a handful of reserved "#" keys the engine gives meaning to. Inheritance
    //  is defined as merging the base in first and letting the derived entries
    //  overwrite it, so a flattened copy behaves the same as a chain walked at
    //  every call and costs nothing per call.
    // ---------------------------------------------------------------------

    const std::string KEY_BASE = "#base";
    const std::string KEY_CREATE = "#create";
    const std::string KEY_TYPE = "#type";
    const std::string KEY_FLAGS = "#flags";

    // How deep a #base chain may go before we assume it points at itself.
    const int MAX_BASE_DEPTH = 32;

    // Copies the key-value pairs of a prototype into out. A prototype is
    // either a hashmap or an array of [key, value] pairs, because both forms
    // read naturally at a call site.
    bool collect_pairs(
        runtime& runtime,
        value::cref prototype,
        std::unordered_map<sqf::runtime::value, sqf::runtime::value>& out)
    {
        if (prototype.is<t_hashmap>())
        {
            for (auto& it : prototype.data<d_hashmap>()->map()) { out[it.first] = it.second; }
            return true;
        }
        if (!prototype.is<t_array>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                0, t_array(), prototype.type()));
            return false;
        }
        auto arr = prototype.data<d_array>();
        for (size_t i = 0; i < arr->size(); i++)
        {
            auto& it = arr->at(i);
            if (!it.is<t_array>())
            {
                runtime.__logmsg(err::ExpectedArrayTypeMissmatch(
                    runtime.context_active().current_frame().diag_info_from_position(),
                    i, t_array(), it.type()));
                return false;
            }
            auto pair = it.data<d_array>();
            if (pair->size() != 2)
            {
                runtime.__logmsg(err::ExpectedArraySizeMissmatch(
                    runtime.context_active().current_frame().diag_info_from_position(),
                    2, 2, pair->size()));
                return false;
            }
            out[pair->at(0)] = pair->at(1);
        }
        return true;
    }

    // Flattens a prototype and everything it inherits from into one map. The
    // base goes in first so that the derived entries win, which is the same
    // outcome as looking a name up along the chain.
    bool flatten_prototype(
        runtime& runtime,
        value::cref prototype,
        std::unordered_map<sqf::runtime::value, sqf::runtime::value>& out,
        int depth)
    {
        if (depth > MAX_BASE_DEPTH)
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                0, MAX_BASE_DEPTH, depth));
            return false;
        }

        std::unordered_map<sqf::runtime::value, sqf::runtime::value> own;
        if (!collect_pairs(runtime, prototype, own)) { return false; }

        auto base = own.find(value(KEY_BASE));
        if (base != own.end() && !base->second.empty())
        {
            if (!flatten_prototype(runtime, base->second, out, depth + 1)) { return false; }
        }
        for (auto& it : own) { out[it.first] = it.second; }
        return true;
    }

    // Runs a method and then discards its result in favour of a value the
    // operator wants to yield instead - the constructed object, in the only
    // case that needs it.
    class behavior_yield : public frame::behavior
    {
    private:
        sqf::runtime::value m_result;
    public:
        behavior_yield(sqf::runtime::value result) : m_result(result) {}
        virtual result enact(sqf::runtime::runtime& runtime, sqf::runtime::frame& frame) override
        {
            runtime.context_active().pop_value();
            runtime.context_active().push_value(m_result);
            return result::ok;
        }
    };

    value createhashmap_(runtime& runtime)
    {
        return std::make_shared<d_hashmap>();
    }
    value createhashmapfromarray_array(runtime& runtime, value::cref right)
    {
        std::unordered_map<sqf::runtime::value, sqf::runtime::value> hashmap;
        auto arr = right.data<d_array>();
        for (size_t i = 0; i < arr->size(); i++)
        {
            auto& it = arr->at(i);
            if (it.is<t_array>())
            {
                auto subArr = it.data<d_array>();
                if (subArr->size() == 2)
                {
                    auto& key = subArr->at(0);
                    auto& value = subArr->at(1);
                    // ToDo: Check key-type matches
                    hashmap[key] = value;
                }
                else
                {
                    runtime.__logmsg(err::ExpectedArraySizeMissmatch(
                        runtime.context_active().current_frame().diag_info_from_position(),
                        2,
                        2,
                        subArr->size()));
                }
            }
            else
            {
                runtime.__logmsg(err::ExpectedArrayTypeMissmatch(
                    runtime.context_active().current_frame().diag_info_from_position(),
                    i,
                    t_array(),
                    it.type()));
            }
        }
        return std::make_shared<d_hashmap>(hashmap);
    }
    value set_hashmap_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto data = left.data<d_hashmap>();
        auto arr = right.data<d_array>();
        if (arr->size() == 2)
        {
            auto& key = arr->at(0);
            auto& value = arr->at(1);
            // ToDo: Check key-type matches
            data->map()[key] = value;
        }
        else
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                2,
                2,
                arr->size()));
        }
        return {};
    }
    value get_hashmap_any(runtime& runtime, value::cref left, value::cref right)
    {
        auto data = left.data<d_hashmap>();
        auto res = data->map().find(right);
        if (res != data->map().end())
        {
            return res->second;
        }
        else
        {
            // ToDo: Log warning about key not found
        }
        return {};
    }
    value deleteat_hashmap_any(runtime& runtime, value::cref left, value::cref right)
    {
        auto data = left.data<d_hashmap>();
        auto res = data->map().find(right);
        if (res != data->map().end())
        {
            auto val = *res;
            data->map().erase(right);
            return val.second;
        }
        else
        {
            // ToDo: Log warning about key not found
        }
        return {};
    }
    value in_any_hashmap(runtime& runtime, value::cref left, value::cref right)
    {
        auto data = right.data<d_hashmap>();
        return data->map().find(left) != data->map().end();
    }
    value count_hashmap(runtime& runtime, value::cref right)
    {
        return right.data<d_hashmap>()->map().size();
    }
    value keys_hashmap(runtime& runtime, value::cref right)
    {
        std::vector<value> keys;
        auto data = right.data<d_hashmap>();
        for (auto& it : data->map())
        {
            keys.push_back(it.first);
        }
        return std::make_shared<d_array>(keys);
    }

    value values_hashmap(runtime& runtime, value::cref right)
    {
        std::vector<value> values;
        for (auto& it : right.data<d_hashmap>()->map()) { values.push_back(it.second); }
        return std::make_shared<d_array>(values);
    }

    // left merge [source, overwriteExisting]
    // source may be a hashmap or an array of [key, value] pairs. Without
    // overwriteExisting a key the target already holds is left alone, which is
    // what makes merge usable for filling in defaults.
    value merge_hashmap_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto arr = right.data<d_array>();
        if (arr->size() < 1 || arr->size() > 2)
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                1, 2, arr->size()));
            return {};
        }

        bool overwrite = false;
        if (arr->size() == 2)
        {
            auto& flag = arr->at(1);
            if (!flag.is<t_boolean>())
            {
                runtime.__logmsg(err::ExpectedArrayTypeMissmatch(
                    runtime.context_active().current_frame().diag_info_from_position(),
                    1, t_boolean(), flag.type()));
                return {};
            }
            overwrite = flag.data<d_boolean, bool>();
        }

        std::unordered_map<sqf::runtime::value, sqf::runtime::value> incoming;
        if (!collect_pairs(runtime, arr->at(0), incoming)) { return {}; }

        auto& target = left.data<d_hashmap>()->map();
        for (auto& it : incoming)
        {
            if (overwrite || target.find(it.first) == target.end()) { target[it.first] = it.second; }
        }
        return {};
    }

    // createHashMapObject [prototype, argsForCreate]
    value createhashmapobject_array(runtime& runtime, value::cref right)
    {
        auto arr = right.data<d_array>();
        if (arr->size() < 1 || arr->size() > 2)
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                1, 2, arr->size()));
            return {};
        }

        std::unordered_map<sqf::runtime::value, sqf::runtime::value> members;
        if (!flatten_prototype(runtime, arr->at(0), members, 0)) { return {}; }

        auto object = std::make_shared<d_hashmap>(members);
        value objectValue(object);

        auto create = object->map().find(value(KEY_CREATE));
        if (create == object->map().end() || !create->second.is<t_code>())
        {
            return objectValue;
        }

        // The constructor runs, but the object is what the expression yields.
        frame f(
            runtime.default_value_scope(),
            create->second.data<d_code, instruction_set>(),
            std::make_shared<behavior_yield>(objectValue));
        f["_self"] = objectValue;
        f["_this"] = arr->size() == 2 ? arr->at(1) : value{};
        runtime.context_active().push_frame(f);
        return {};
    }

    // object call ["methodName"] or object call ["methodName", arguments]
    value call_hashmap_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto arr = right.data<d_array>();
        if (arr->size() < 1 || arr->size() > 2)
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                1, 2, arr->size()));
            return {};
        }

        auto object = left.data<d_hashmap>();
        auto method = object->map().find(arr->at(0));
        if (method == object->map().end() || !method->second.is<t_code>())
        {
            // A name that is not a method is not an error the caller can act
            // on mid-expression; nil is what reading a missing key gives too.
            return {};
        }

        frame f(runtime.default_value_scope(), method->second.data<d_code, instruction_set>());
        f["_self"] = left;
        f["_this"] = arr->size() == 2 ? arr->at(1) : value{};
        runtime.context_active().push_frame(f);
        return {};
    }

    value plus_hashmap(runtime& runtime, value::cref right)
    {
        std::unordered_map<sqf::runtime::value, sqf::runtime::value> hashmap = right.data<d_hashmap>()->map();
        return std::make_shared<d_hashmap>(hashmap);
    }
}

void sqf::operators::ops_hashmap(::sqf::runtime::runtime& runtime)
{
    using namespace sqf::runtime::sqfop;
    runtime.register_sqfop(nular("createHashMap", "Creates a hashmap.", createhashmap_));
    runtime.register_sqfop(unary("createHashMapFromArray", t_array(), "Creates a hashmap from an key-value pair array ([[key, value], ...]).", createhashmapfromarray_array));
    runtime.register_sqfop(binary(4, "set", t_hashmap(), t_array(), "Assigns a value to a hashmap.", set_hashmap_array));
    runtime.register_sqfop(binary(4, "get", t_hashmap(), t_any(), "Receives a value from a hashmap.", get_hashmap_any));
    runtime.register_sqfop(binary(4, "deleteAt", t_hashmap(), t_any(), "Removes a key-value pair from a hashmap.", deleteat_hashmap_any));
    runtime.register_sqfop(binary(4, "in", t_any(), t_hashmap(), "Checks if a given key is inside a value.", in_any_hashmap));
    runtime.register_sqfop(unary("count", t_hashmap(), "Returns the number of elements inside a hashmap.", count_hashmap));
    runtime.register_sqfop(unary("keys", t_hashmap(), "Returns the keys of a hashmap.", keys_hashmap));
    runtime.register_sqfop(unary("+", t_hashmap(), "Returns a copy of the hashmap.", plus_hashmap));
    runtime.register_sqfop(unary("values", t_hashmap(), "Returns the values of a hashmap.", values_hashmap));
    runtime.register_sqfop(binary(4, "merge", t_hashmap(), t_array(), "Merges [source, overwriteExisting] into a hashmap.", merge_hashmap_array));
    runtime.register_sqfop(unary("createHashMapObject", t_array(), "Creates a hashmap object from [prototype, argsForCreate].", createhashmapobject_array));
    runtime.register_sqfop(binary(4, "call", t_hashmap(), t_array(), "Calls a method of a hashmap object with [methodName, arguments].", call_hashmap_array));
}
