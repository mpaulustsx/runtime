#pragma once
#include "../runtime/data.h"
#include "../runtime/type.h"
#include "../runtime/value.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <vector>
#include <algorithm>


namespace sqf
{
    namespace runtime
    {
        struct t_hashmap : public type::extend<t_hashmap> { t_hashmap() : extend() {} static const std::string name() { return "HASHMAP"; } };
        class runtime;
    }
    namespace types
    {
        class d_hashmap : public sqf::runtime::data
        {
        private:
            std::unordered_map<sqf::runtime::value, sqf::runtime::value> m_map;
        public:
            using data_type = sqf::runtime::t_hashmap;
        protected:
            virtual bool do_equals(std::shared_ptr<data> other, bool invariant) const
            {
                auto& other_map = std::static_pointer_cast<d_hashmap>(other)->map();
                auto& self_map = m_map;

                return other_map == self_map;
            }
        public:
            d_hashmap() = default;
            d_hashmap(std::unordered_map<sqf::runtime::value, sqf::runtime::value> map) : m_map(map) {}

            sqf::runtime::type type() const override { return data_type(); }
            // Guards hash()/to_string_sqf()/to_string() against self- or
            // cross-referential hashmap graphs (e.g. two objects that hold
            // back-references into each other) recursing without bound -
            // same rationale as data::equals()'s pair guard, but these are
            // unary (only `this` matters, no "other" to pair with).
            virtual std::size_t hash() const override
            {
                thread_local std::vector<const d_hashmap*> s_hash_in_progress;
                if (std::find(s_hash_in_progress.begin(), s_hash_in_progress.end(), this) != s_hash_in_progress.end())
                {
                    return 0x9e3779b9;
                }
                s_hash_in_progress.push_back(this);
                size_t hash = 0x9e3779b9;
                for (auto& it : m_map)
                {
                    hash ^= std::hash<sqf::runtime::value>()(it.first) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                    hash ^= std::hash<sqf::runtime::value>()(it.second) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                }
                s_hash_in_progress.pop_back();
                return hash;
            }

            std::string to_string_sqf() const override
            {
                thread_local std::vector<const d_hashmap*> s_tostring_sqf_in_progress;
                if (std::find(s_tostring_sqf_in_progress.begin(), s_tostring_sqf_in_progress.end(), this) != s_tostring_sqf_in_progress.end())
                {
                    return "[]";
                }
                s_tostring_sqf_in_progress.push_back(this);
                std::stringstream sstream;
                sstream << "[";
                if (m_map.size() > 0)
                {
                    for (auto it : m_map)
                    {
                        sstream << "[" << it.first.to_string_sqf() << "," << it.second.to_string_sqf() << "]" << ",";
                    }
                    sstream.seekp(-1, std::ios_base::end);
                }
                sstream << "]";
                s_tostring_sqf_in_progress.pop_back();
                return sstream.str();
            }
            std::string to_string() const override
            {
                thread_local std::vector<const d_hashmap*> s_tostring_in_progress;
                if (std::find(s_tostring_in_progress.begin(), s_tostring_in_progress.end(), this) != s_tostring_in_progress.end())
                {
                    return "{...}";
                }
                s_tostring_in_progress.push_back(this);
                std::stringstream sstream;
                sstream << "[";
                if (m_map.size() > 0)
                {
                    for (auto it : m_map)
                    {
                        sstream << it.first.to_string() << ": " << it.second.to_string() << ",";
                    }
                    sstream.seekp(-1, std::ios_base::end);
                }
                sstream << "}";
                s_tostring_in_progress.pop_back();
                return sstream.str();
            }

            std::unordered_map<sqf::runtime::value, sqf::runtime::value>& map() { return m_map; }
        };
    }

    namespace operators
    {
        void ops_hashmap(::sqf::runtime::runtime& runtime);
    }
}