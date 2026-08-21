#pragma once
#include "../runtime/data.h"
#include "../runtime/type.h"
#include "../runtime/value.h"
#include "../runtime/vec.h"
#include "../runtime/value_scope.h"
#include "../runtime/runtime.h"
#include "../runtime/d_scalar.h"

#include <string>
#include <string_view>
#include <sstream>
#include <memory>
#include <vector>
#include <algorithm>

namespace sqf
{
    namespace types
    {
        // Minimal backing for createLocation/location handles. No real
        // terrain/location database is loaded in this headless engine -
        // this just tracks the handful of properties a script can set and
        // read back faithfully (position, type, size), the same
        // minimal-but-real bar sqf::types::object holds to. A variable
        // space is inherited the same way object gets one, so
        // getVariable/setVariable work on a location exactly like they do
        // on an object (Vindicta's SaveSystem/JNA code uses locations as
        // throwaway variable-space holders for this reason).
        class location : public std::enable_shared_from_this<location>, public ::sqf::runtime::value_scope
        {
        public:
            class location_storage : public sqf::runtime::runtime::datastorage
            {
            private:
                std::vector<std::shared_ptr<location>> m_inner;
            public:
                virtual ~location_storage() override {}
                void push_back(std::shared_ptr<location> loc) { m_inner.push_back(std::move(loc)); }
                void erase(std::shared_ptr<location> loc)
                {
                    auto it = std::find(m_inner.begin(), m_inner.end(), loc);
                    if (it != m_inner.end()) { *it = m_inner.back(); m_inner.pop_back(); }
                }
            };
        private:
            std::string m_type;
            ::sqf::runtime::vec3 m_position;
            float m_direction;
            float m_size_a;
            float m_size_b;
            std::string m_text;

            location(std::string type, ::sqf::runtime::vec3 pos, float a, float b) :
                m_type(std::move(type)), m_position(pos), m_direction(0.0f), m_size_a(a), m_size_b(b), m_text()
            {
            }
        public:
            static std::shared_ptr<location> create(::sqf::runtime::runtime& runtime, std::string type, ::sqf::runtime::vec3 pos, float a, float b)
            {
                auto& storage = runtime.storage<location_storage>();
                auto sp = std::shared_ptr<location>(new location(std::move(type), pos, a, b));
                storage.push_back(sp);
                return sp;
            }
            void destroy(::sqf::runtime::runtime& runtime)
            {
                runtime.storage<location_storage>().erase(shared_from_this());
            }

            std::string_view type() const { return m_type; }

            ::sqf::runtime::vec3 position() const { return m_position; }
            void position(::sqf::runtime::vec3 vec) { m_position = vec; }

            float direction() const { return m_direction; }
            void direction(float val) { m_direction = val; }

            float size_a() const { return m_size_a; }
            float size_b() const { return m_size_b; }
            void size(float a, float b) { m_size_a = a; m_size_b = b; }

            std::string_view text() const { return m_text; }
            void text(std::string val) { m_text = std::move(val); }
        };

        class d_location : public sqf::runtime::data
        {
        public:
            using data_type = sqf::types::t_location;
        private:
            std::weak_ptr<location> m_value;
        protected:
            bool do_equals(std::shared_ptr<sqf::runtime::data> other, bool invariant) const override
            {
                return value().get() == std::static_pointer_cast<d_location>(other)->value().get();
            }
        public:
            d_location() = default;
            d_location(std::weak_ptr<location> value) : m_value(value) {}

            std::string to_string_sqf() const override
            {
                auto loc = value();
                if (!loc)
                {
                    return "NULL";
                }
                std::stringstream sstream;
                sstream << static_cast<const void*>(loc.get()) << "# " << loc->type();
                return sstream.str();
            }
            std::string to_string() const override { return to_string_sqf(); }

            sqf::runtime::type type() const override { return data_type(); }
            virtual std::size_t hash() const override { return 0; }

            bool is_null() const { return m_value.expired(); }

            std::shared_ptr<::sqf::types::location> value() const
            {
                if (m_value.expired())
                {
                    return {};
                }
                return m_value.lock();
            }
            void value(std::shared_ptr<::sqf::types::location> val) { m_value = val; }

            operator std::shared_ptr<::sqf::types::location>() { return value(); }
        };
        template<>
        inline std::shared_ptr<sqf::runtime::data> to_data<std::shared_ptr<sqf::types::location>>(std::shared_ptr<sqf::types::location> value)
        {
            return std::make_shared<d_location>(value);
        }
    }
}
