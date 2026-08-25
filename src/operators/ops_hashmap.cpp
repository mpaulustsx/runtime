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
#include <algorithm>
#include <cctype>

using namespace std::string_literals;



namespace err = logmessage::runtime;
using namespace sqf::runtime;
using namespace sqf::types;

namespace
{
    // -------------------------------------------------------------------
    //  Small helpers shared by the plain hashmap ops and the object ops
    // -------------------------------------------------------------------

    std::string to_lower(const std::string& text)
    {
        std::string out = text;
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });
        return out;
    }

    // "#flags" holds an array of case-insensitive strings. Reading it back
    // out is a single find(), so an ordinary hashmap that never used it pays
    // almost nothing for the check.
    bool has_flag(std::unordered_map<sqf::runtime::value, sqf::runtime::value>& map, const std::string& flagLower)
    {
        auto it = map.find(value(std::string("#flags")));
        if (it == map.end() || !it->second.is<t_array>()) { return false; }
        auto arr = it->second.data<d_array>();
        for (size_t i = 0; i < arr->size(); i++)
        {
            auto& entry = arr->at(i);
            if (entry.is<t_string>() && to_lower(entry.data<d_string, std::string>()) == flagLower) { return true; }
        }
        return false;
    }
    bool is_sealed(std::unordered_map<sqf::runtime::value, sqf::runtime::value>& map) { return has_flag(map, "sealed"); }
    bool is_no_copy(std::unordered_map<sqf::runtime::value, sqf::runtime::value>& map) { return has_flag(map, "nocopy"); }

    // The reference does not accept a HashMap as a HashMap key - it is a
    // mutable reference type, not a hashable value, and the real engine
    // throws on it rather than silently accepting it the way this VM's own
    // std::unordered_map<value, value> otherwise would (d_hashmap::hash()
    // happily hashes one, cycle guard and all). Left unchecked, code that
    // keys a cache by a live hashmap-storage object passes every SQF-VM run
    // and only surfaces as a live-server crash - exactly the gap that let
    // SensorGarrisonTargets and IntelDatabase's own caches ship broken.
    // Scoped to that one evidenced case, not every key restriction the
    // reference might also impose - no evidence here for any of the others.
    bool is_valid_hashmap_key(value::cref key) { return !key.is<t_hashmap>(); }

    void log_invalid_hashmap_key(runtime& runtime, const std::string& source, value::cref key)
    {
        runtime.__logmsg(err::ErrorMessage(
            runtime.context_active().current_frame().diag_info_from_position(),
            source, "a hashmap cannot itself be used as a hashmap key: "s + key.to_string()));
    }

    // A resolved "#create"/"#clone"/"#delete" chain is always stored as an
    // Array of Code, even when it holds just one entry - see the comment on
    // resolve_code_chain below for why. A hashmap assembled by hand rather
    // than through createHashMapObject may still set the key to a bare Code
    // though, so reading it back stays lenient about the shape.
    std::vector<sqf::runtime::value> collect_code_chain(sqf::runtime::value::cref field)
    {
        std::vector<sqf::runtime::value> chain;
        if (field.empty()) { return chain; }
        if (field.is<t_code>()) { chain.push_back(field); return chain; }
        if (field.is<t_array>())
        {
            auto arr = field.data<d_array>();
            for (size_t i = 0; i < arr->size(); i++)
            {
                if (arr->at(i).is<t_code>()) { chain.push_back(arr->at(i)); }
            }
        }
        return chain;
    }

    // ---------------------------------------------------------------------
    //  Hashmap objects
    //
    //  A hashmap object is an ordinary hashmap whose values may be code,
    //  plus a handful of reserved "#" keys the engine gives meaning to.
    //  Inheritance is resolved once, at construction, by flattening the
    //  #base chain: regular keys behave like merge with overwriteExisting,
    //  the derived entry wins. #create, #clone and #delete are the stated
    //  exception - "will be merged together and executed in sequence" -
    //  so they are collected into an ordered chain, base first, instead of
    //  being overwritten. #type is its own exception again: "on
    //  inheritance types will be merged into an Array"; with no
    //  inheritance at all it is kept exactly as declared.
    // ---------------------------------------------------------------------

    const std::string KEY_BASE = "#base";
    const std::string KEY_CREATE = "#create";
    const std::string KEY_CLONE = "#clone";
    const std::string KEY_DELETE = "#delete";
    const std::string KEY_TYPE = "#type";

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

    struct flatten_result
    {
        std::vector<sqf::runtime::value> create_chain;
        std::vector<sqf::runtime::value> clone_chain;
        std::vector<sqf::runtime::value> delete_chain;
        std::vector<sqf::runtime::value> type_values;
        bool used_inheritance = false;
    };

    // Flattens a prototype and everything it inherits from into one map,
    // base first so the derived entries win - the same outcome merge with
    // overwriteExisting would give. #create/#clone/#delete/#type are pulled
    // out of the ordinary overwrite path and appended to their own ordered
    // lists as they are encountered, which - because the base is always
    // visited before the level that declares it - naturally comes out base
    // to derived, matching the order the reference's own worked example
    // logs constructor calls in.
    bool flatten_prototype(
        runtime& runtime,
        value::cref prototype,
        std::unordered_map<sqf::runtime::value, sqf::runtime::value>& out,
        flatten_result& result,
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
            result.used_inheritance = true;
            if (!flatten_prototype(runtime, base->second, out, result, depth + 1)) { return false; }
        }

        for (auto& it : own)
        {
            auto key = it.first;
            if (key.is<t_string>())
            {
                auto name = key.data<d_string, std::string>();
                if (name == KEY_BASE) { continue; }
                if (name == KEY_CREATE) { auto c = collect_code_chain(it.second); result.create_chain.insert(result.create_chain.end(), c.begin(), c.end()); continue; }
                if (name == KEY_CLONE)  { auto c = collect_code_chain(it.second); result.clone_chain.insert(result.clone_chain.end(), c.begin(), c.end()); continue; }
                if (name == KEY_DELETE) { auto c = collect_code_chain(it.second); result.delete_chain.insert(result.delete_chain.end(), c.begin(), c.end()); continue; }
                if (name == KEY_TYPE)   { result.type_values.push_back(it.second); continue; }
            }
            out[key] = it.second;
        }
        return true;
    }

    // Always an Array, even for a chain of one: a hand-inspected object then
    // has one shape to read regardless of whether inheritance was involved,
    // and a future copy or delete never has to guess which shape it got.
    sqf::runtime::value resolve_code_chain(const std::vector<sqf::runtime::value>& chain)
    {
        if (chain.empty()) { return {}; }
        return std::make_shared<d_array>(chain);
    }

    // Unlike the chains above, the reference ties the Array specifically to
    // "on inheritance": a #type declared with no #base anywhere is kept
    // exactly as given (typically a String), and only turns into an Array
    // once a base actually contributed to it.
    sqf::runtime::value resolve_type(const flatten_result& result)
    {
        if (result.type_values.empty()) { return {}; }
        if (!result.used_inheritance) { return result.type_values.back(); }
        return std::make_shared<d_array>(result.type_values);
    }

    // Runs a chain of Code blocks in sequence - each with the same _self and
    // _this bound throughout, since neither is reset between blocks the
    // engine's own inheritance section groups together as one merged
    // constructor - discards what each one returns, then yields a fixed
    // final value once the chain is spent. The same shape serves #create's
    // constructed object and #clone's already-made copy; only what is
    // chained and what is yielded differ.
    //
    // Chaining relies on frame::behavior::result::exchange: a frame's
    // exit behaviour may hand back a different instruction_set for the
    // frame to keep running, in place of the one it was built with. Because
    // exchange does not clear the frame's variable scope (only seek_start
    // does), _self/_this set once before the first block stay bound for
    // every block after it.
    class behavior_run_chain_then_yield : public frame::behavior
    {
    private:
        std::vector<sqf::runtime::value> m_chain;
        size_t m_index;
        sqf::runtime::value m_final;
    public:
        behavior_run_chain_then_yield(std::vector<sqf::runtime::value> chain, sqf::runtime::value final_value)
            : m_chain(std::move(chain)), m_index(0), m_final(final_value) {}

        virtual sqf::runtime::instruction_set get_instruction_set(sqf::runtime::frame& frame) override
        {
            return m_chain[m_index].data<d_code, instruction_set>();
        }
        virtual result enact(sqf::runtime::runtime& runtime, sqf::runtime::frame& frame) override
        {
            // Discard what the block that just finished returned; only the
            // final value this chain was built to produce matters here.
            runtime.context_active().pop_value();
            m_index++;
            if (m_index < m_chain.size()) { return result::exchange; }
            runtime.context_active().push_value(m_final);
            return result::ok;
        }
    };

    // createHashMapObject [prototype, constructorArguments]
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
        flatten_result resolved;
        if (!flatten_prototype(runtime, arr->at(0), members, resolved, 0)) { return {}; }

        auto insert_if_present = [&members](const std::string& key, sqf::runtime::value value) {
            if (!value.empty()) { members[key] = value; }
        };
        insert_if_present(KEY_CREATE, resolve_code_chain(resolved.create_chain));
        insert_if_present(KEY_CLONE, resolve_code_chain(resolved.clone_chain));
        // #delete is stored resolved for a future implementation of the
        // destructor itself, which needs cooperation from the runtime's
        // execution loop (see the type checker/PR notes) and is not wired
        // up to anything yet - nothing currently triggers it.
        insert_if_present(KEY_DELETE, resolve_code_chain(resolved.delete_chain));
        insert_if_present(KEY_TYPE, resolve_type(resolved));

        auto object = std::make_shared<d_hashmap>(members);
        value objectValue(object);

        if (resolved.create_chain.empty()) { return objectValue; }

        frame f(
            runtime.default_value_scope(),
            resolved.create_chain[0].data<d_code, instruction_set>(),
            std::make_shared<behavior_run_chain_then_yield>(resolved.create_chain, objectValue));
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
                    if (!is_valid_hashmap_key(key)) { log_invalid_hashmap_key(runtime, "createHashMapFromArray"s, key); continue; }
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
    // keysArray createHashMapFromArray valuesArray
    //
    // Per the reference the two arrays do not need to be the same length:
    // the result has exactly one entry per key, a value missing at that
    // index becomes nil, and a values entry past the end of keys is simply
    // never reached rather than starting a pair of its own - confirmed by
    // the reference's own example, where a four-element values array against
    // a three-element keys array drops the fourth value entirely.
    value createhashmapfromarray_array_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto keys = left.data<d_array>();
        auto values = right.data<d_array>();
        std::unordered_map<sqf::runtime::value, sqf::runtime::value> hashmap;
        for (size_t i = 0; i < keys->size(); i++)
        {
            auto& key = keys->at(i);
            if (!is_valid_hashmap_key(key)) { log_invalid_hashmap_key(runtime, "createHashMapFromArray"s, key); continue; }
            hashmap[key] = i < values->size() ? values->at(i) : sqf::runtime::value{};
        }
        return std::make_shared<d_hashmap>(hashmap);
    }
    value set_hashmap_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto data = left.data<d_hashmap>();
        auto arr = right.data<d_array>();
        if (arr->size() != 2)
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                2,
                2,
                arr->size()));
            return {};
        }

        auto& key = arr->at(0);
        auto& value = arr->at(1);

        if (!is_valid_hashmap_key(key)) { log_invalid_hashmap_key(runtime, "set"s, key); return {}; }

        // "sealed" forbids adding a key, not editing one that is already
        // there - an existing key still goes through below. This is a
        // warning, not a fatal error: an error-level __logmsg halts the
        // whole script, and refusing to add one key should not do that.
        if (data->map().find(key) == data->map().end() && is_sealed(data->map()))
        {
            runtime.__logmsg(err::WarningMessage(
                runtime.context_active().current_frame().diag_info_from_position(),
                "set"s, "hashmap is sealed, refusing to add key '" + right.to_string() + "'"s));
            return {};
        }

        data->map()[key] = value;
        return {};
    }
    value get_hashmap_any(runtime& runtime, value::cref left, value::cref right)
    {
        if (!is_valid_hashmap_key(right)) { log_invalid_hashmap_key(runtime, "get"s, right); return {}; }

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
        if (!is_valid_hashmap_key(right)) { log_invalid_hashmap_key(runtime, "deleteAt"s, right); return {}; }

        auto data = left.data<d_hashmap>();

        if (is_sealed(data->map()))
        {
            runtime.__logmsg(err::WarningMessage(
                runtime.context_active().current_frame().diag_info_from_position(),
                "deleteAt"s, "hashmap is sealed, refusing to remove key '" + right.to_string() + "'"s));
            return {};
        }

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
        if (!is_valid_hashmap_key(left)) { log_invalid_hashmap_key(runtime, "in"s, left); return false; }

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
    // Tied to keys by the reference: "The order of the returned Array
    // corresponds to the order of the Array returned by the keys command."
    // Both walk the same underlying map with nothing in between that could
    // reorder it, so one shared iteration keeps that promise by
    // construction rather than by coincidence.
    value values_hashmap(runtime& runtime, value::cref right)
    {
        std::vector<value> values;
        auto data = right.data<d_hashmap>();
        for (auto& it : data->map())
        {
            values.push_back(it.second);
        }
        return std::make_shared<d_array>(values);
    }

    // Adds every pair of source into target. A key the target already holds
    // is only replaced when overwriteExisting says so, which is what makes
    // merge usable for filling in defaults.
    void merge_into(
        std::unordered_map<sqf::runtime::value, sqf::runtime::value>& target,
        std::unordered_map<sqf::runtime::value, sqf::runtime::value>& source,
        bool overwrite)
    {
        for (auto& it : source)
        {
            if (overwrite || target.find(it.first) == target.end()) { target[it.first] = it.second; }
        }
    }

    // hashMap1 merge hashMap2
    value merge_hashmap_hashmap(runtime& runtime, value::cref left, value::cref right)
    {
        merge_into(left.data<d_hashmap>()->map(), right.data<d_hashmap>()->map(), false);
        return {};
    }

    // hashMap1 merge [hashMap2, overwriteExisting]
    //
    // The reference's second syntax takes a HashMap as hashMap2, the same as
    // the first - not an array of pairs. Accepting a pair array here as well
    // would let a mission pass something that fails in the real engine while
    // this one quietly succeeds, which is the one direction a test runner
    // must not be lenient in.
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

        auto& source = arr->at(0);
        if (!source.is<t_hashmap>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                0, t_hashmap(), source.type()));
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

        merge_into(left.data<d_hashmap>()->map(), source.data<d_hashmap>()->map(), overwrite);
        return {};
    }

    // Reads [key, fallback, setDefault] the way both getOrDefault commands
    // take it - only key is required, the rest defaults the same way the
    // reference documents its own optional parameters.
    void read_default_args(
        value::cref right,
        sqf::runtime::value& key,
        sqf::runtime::value& fallback,
        bool& store)
    {
        auto arr = right.data<d_array>();
        key = arr->size() >= 1 ? arr->at(0) : sqf::runtime::value{};
        fallback = arr->size() >= 2 ? arr->at(1) : sqf::runtime::value{};
        store = arr->size() >= 3 && arr->at(2).is<t_boolean>() && arr->at(2).data<d_boolean, bool>();
    }

    // hashMap getOrDefault [key, defaultValue, setDefault]
    value getordefault_hashmap_array(runtime& runtime, value::cref left, value::cref right)
    {
        sqf::runtime::value key, fallback;
        bool store;
        read_default_args(right, key, fallback, store);
        if (!is_valid_hashmap_key(key)) { log_invalid_hashmap_key(runtime, "getOrDefault"s, key); return {}; }

        auto data = left.data<d_hashmap>();
        auto found = data->map().find(key);
        if (found != data->map().end()) { return found->second; }

        if (store) { data->map()[key] = fallback; }
        return fallback;
    }

    // Keeps what the default code returned, and writes it into the map
    // first when the caller asked for that - never re-running the code to
    // get the value it stores, only the one it already produced.
    class behavior_default_call : public frame::behavior
    {
    private:
        std::shared_ptr<d_hashmap> m_map;
        sqf::runtime::value m_key;
        bool m_store;
    public:
        behavior_default_call(std::shared_ptr<d_hashmap> map, sqf::runtime::value key, bool store)
            : m_map(map), m_key(key), m_store(store) {}
        virtual result enact(sqf::runtime::runtime& runtime, sqf::runtime::frame& frame) override
        {
            auto res = runtime.context_active().pop_value();
            sqf::runtime::value produced = res.has_value() ? *res : sqf::runtime::value{};
            if (m_store) { m_map->map()[m_key] = produced; }
            runtime.context_active().push_value(produced);
            return result::ok;
        }
    };

    // hashMap getOrDefaultCall [key, defaultCode, setDefault]
    //
    // The code only runs when the key is missing, which is the point of the
    // Call variant over plain getOrDefault: building the default may be
    // expensive, and a key that already exists must never pay for it. With
    // no default code at all - the documented default, nil - a missing key
    // simply yields nil rather than treating the omission as an error.
    value getordefaultcall_hashmap_array(runtime& runtime, value::cref left, value::cref right)
    {
        sqf::runtime::value key, fallback;
        bool store;
        read_default_args(right, key, fallback, store);
        if (!is_valid_hashmap_key(key)) { log_invalid_hashmap_key(runtime, "getOrDefaultCall"s, key); return {}; }

        auto data = left.data<d_hashmap>();
        auto found = data->map().find(key);
        if (found != data->map().end()) { return found->second; }

        if (fallback.empty()) { return {}; }
        if (!fallback.is<t_code>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(
                runtime.context_active().current_frame().diag_info_from_position(),
                1, t_code(), fallback.type()));
            return {};
        }

        frame f(
            runtime.default_value_scope(),
            fallback.data<d_code, instruction_set>(),
            std::make_shared<behavior_default_call>(data, key, store));
        // The key is what the code has to work from; passing it costs
        // nothing for code that ignores _this.
        f["_this"] = key;
        runtime.context_active().push_frame(f);
        return {};
    }

    // + hashMapObject
    //
    // "noCopy" forbids this outright. Otherwise the map is copied field by
    // field first - the same behaviour a plain hashmap has always had - and
    // only then does a #clone chain, if the object declares one, run against
    // the finished copy; #clone exists to let an object react to being
    // copied, not to build the copy itself.
    value plus_hashmap(runtime& runtime, value::cref right)
    {
        auto& source = right.data<d_hashmap>()->map();
        if (is_no_copy(source))
        {
            runtime.__logmsg(err::ErrorMessage(
                runtime.context_active().current_frame().diag_info_from_position(),
                "+"s, "hashmap object has the noCopy flag and cannot be copied"s));
            return {};
        }

        // Per the reference, an Array held as a key or a value is
        // deep-copied along with the hashmap itself - the same rule plain
        // Array "+" already follows for its own elements - so that editing
        // one copy's nested array never reaches back into the other's.
        std::unordered_map<sqf::runtime::value, sqf::runtime::value> hashmap;
        for (auto& it : source)
        {
            auto copy_if_array = [](sqf::runtime::value::cref v) -> sqf::runtime::value {
                return v.is<t_array>() ? sqf::runtime::value(v.data<d_array>()->copy_deep()) : v;
            };
            hashmap[copy_if_array(it.first)] = copy_if_array(it.second);
        }
        auto copy = std::make_shared<d_hashmap>(hashmap);
        value copyValue(copy);

        auto found = copy->map().find(value(KEY_CLONE));
        std::vector<sqf::runtime::value> cloneChain =
            found != copy->map().end() ? collect_code_chain(found->second) : std::vector<sqf::runtime::value>{};
        if (cloneChain.empty()) { return copyValue; }

        frame f(
            runtime.default_value_scope(),
            cloneChain[0].data<d_code, instruction_set>(),
            std::make_shared<behavior_run_chain_then_yield>(cloneChain, copyValue));
        f["_self"] = copyValue;
        runtime.context_active().push_frame(f);
        return {};
    }
}

void sqf::operators::ops_hashmap(::sqf::runtime::runtime& runtime)
{
    using namespace sqf::runtime::sqfop;
    runtime.register_sqfop(nular("createHashMap", "Creates a hashmap.", createhashmap_));
    runtime.register_sqfop(unary("createHashMapFromArray", t_array(), "Creates a hashmap from an key-value pair array ([[key, value], ...]).", createhashmapfromarray_array));
    runtime.register_sqfop(binary(4, "createHashMapFromArray", t_array(), t_array(), "Creates a hashmap from a keys array and a values array; a value missing for a key becomes nil.", createhashmapfromarray_array_array));
    runtime.register_sqfop(binary(4, "set", t_hashmap(), t_array(), "Assigns a value to a hashmap.", set_hashmap_array));
    runtime.register_sqfop(binary(4, "get", t_hashmap(), t_any(), "Receives a value from a hashmap.", get_hashmap_any));
    runtime.register_sqfop(binary(4, "getOrDefault", t_hashmap(), t_array(), "Returns the value at [key, defaultValue, setDefault], or the default.", getordefault_hashmap_array));
    runtime.register_sqfop(binary(4, "getOrDefaultCall", t_hashmap(), t_array(), "Returns the value at [key, defaultCode, setDefault], running the code only when the key is missing.", getordefaultcall_hashmap_array));
    runtime.register_sqfop(binary(4, "deleteAt", t_hashmap(), t_any(), "Removes a key-value pair from a hashmap.", deleteat_hashmap_any));
    runtime.register_sqfop(binary(4, "in", t_any(), t_hashmap(), "Checks if a given key is inside a value.", in_any_hashmap));
    runtime.register_sqfop(unary("count", t_hashmap(), "Returns the number of elements inside a hashmap.", count_hashmap));
    runtime.register_sqfop(unary("keys", t_hashmap(), "Returns the keys of a hashmap.", keys_hashmap));
    runtime.register_sqfop(unary("values", t_hashmap(), "Returns the values of a hashmap.", values_hashmap));
    runtime.register_sqfop(unary("+", t_hashmap(), "Returns a copy of the hashmap, running any #clone chain against it.", plus_hashmap));
    runtime.register_sqfop(binary(4, "merge", t_hashmap(), t_hashmap(), "Merges another hashmap in, keeping the keys this one already holds.", merge_hashmap_hashmap));
    runtime.register_sqfop(binary(4, "merge", t_hashmap(), t_array(), "Merges [hashMap, overwriteExisting] into a hashmap.", merge_hashmap_array));
    runtime.register_sqfop(unary("createHashMapObject", t_array(), "Creates a hashmap object from [prototype, constructorArguments].", createhashmapobject_array));
    runtime.register_sqfop(binary(4, "call", t_hashmap(), t_array(), "Calls a method of a hashmap object with [methodName, arguments].", call_hashmap_array));
}
