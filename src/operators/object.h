#pragma once
#include "../runtime/vec.h"
#include "../runtime/confighost.h"
#include "../runtime/value_scope.h"
#include "../runtime/runtime.h"


#include <string>
#include <string_view>
#include <memory>
#include <array>
#include <vector>
#include <algorithm>
#include <set>
#include <utility>
#include <optional>

namespace sqf
{
    namespace runtime
    {
        class value;
    }
    namespace types
    {
        class d_group;
        class d_object;
        class object : public std::enable_shared_from_this<::sqf::types::object>, public ::sqf::runtime::value_scope
        {
        public:
            class object_storage : public sqf::runtime::runtime::datastorage
            {
            private:
                std::vector<std::shared_ptr<object>> m_inner;
                size_t m_id;
                std::shared_ptr<object> m_player;
            public:
                object_storage() : m_inner(), m_id(0) {}
                virtual ~object_storage() override {}
                size_t push_back(std::shared_ptr<object> obj) { m_inner.push_back(obj); return ++m_id; }
                void erase(std::shared_ptr<object> obj) { auto it = std::find(m_inner.begin(), m_inner.end(), obj); if (it != m_inner.end()) { *it = m_inner.back(); m_inner.pop_back(); } }
                std::vector<std::shared_ptr<object>>::iterator begin() { return m_inner.begin(); }
                std::vector<std::shared_ptr<object>>::iterator end() { return m_inner.end(); }
                std::shared_ptr<object> player() { return m_player; }
                void player(std::shared_ptr<object> obj) { m_player = obj; }
            };
            struct configuration_
            {
                size_t transport_soldier;
                bool has_driver;
                bool has_gunner;
                bool has_commander;
            };
            class soldiers_
            {
            private:
                friend class object;
                std::vector<std::shared_ptr<d_object>> m_inner;
                object* m_owner;
                soldiers_(object* owner) : m_owner(owner) {}
            public:
                std::vector<std::shared_ptr<d_object>>::iterator begin() { return m_inner.begin(); }
                std::vector<std::shared_ptr<d_object>>::const_iterator begin() const { return m_inner.begin(); }

                std::vector<std::shared_ptr<d_object>>::iterator end() { return m_inner.end(); }
                std::vector<std::shared_ptr<d_object>>::const_iterator end() const { return m_inner.end(); }

                const std::vector<std::shared_ptr<d_object>>& vector() const { return m_inner; }

                /// <summary>
                /// Attempts to add provided value to the soldiers list.
                /// Will return true if it succeeded, false if the value provided
                /// was null, not an object, not a unit, already inside the vehicle or no slots
                /// are left to occupy.
                /// </summary>
                /// <param name="val"></param>
                /// <returns></returns>
                bool push_back(sqf::runtime::value val);

                /// <summary>
                /// Attempts to add provided objectdata to the soldiers list.
                /// Will return true if it succeeded, false if the value provided
                /// was null, not a unit, already inside the vehicle or no slots are left to occupy.
                /// </summary>
                /// <param name="val"></param>
                /// <returns></returns>
                bool push_back(std::shared_ptr<d_object> val);

                void pop_back();

                void erase(std::shared_ptr<d_object> obj);
            };
        private:
            size_t m_netid;
            sqf::runtime::config m_config;
            bool m_is_vehicle;

            std::string m_varname;
            float m_damage;
            std::shared_ptr<d_group> m_group;

            ::sqf::runtime::vec3 m_position;
            ::sqf::runtime::vec3 m_velocity;

            std::shared_ptr<d_object> m_parent_object;
            std::shared_ptr<d_object> m_driver;
            std::shared_ptr<d_object> m_gunner;
            std::shared_ptr<d_object> m_commander;
            soldiers_ m_soldiers;
            configuration_ m_configuration;

            // Headless-appropriate state for the scripting surface below -
            // no rendering, no real AI/physics, just what a script can set
            // and read back. Given in-class defaults so object(...)'s
            // constructor init-list doesn't need to mention them.
            float m_direction = 0.0f;
            bool m_captive = false;
            bool m_allow_damage = true;
            std::set<std::string> m_disabled_ai;
            std::vector<std::pair<size_t, ::sqf::runtime::value>> m_actions;
            size_t m_next_action_id = 0;
            std::vector<std::pair<std::string, ::sqf::runtime::value>> m_traits;
            std::vector<std::pair<size_t, ::sqf::runtime::value>> m_event_handlers;
            size_t m_next_eh_id = 0;

            // attachTo/attachedTo state - a distinct relationship from
            // m_parent_object above (which models vehicle crew, ie.
            // moveInDriver/vehicle), tracking a plain object-to-object
            // attachment plus its relative offset. No physics simulates the
            // attached object following its parent around - this is just
            // state tracking so isNull (attachedTo x) and similar checks
            // work correctly.
            std::shared_ptr<d_object> m_attached_to;
            ::sqf::runtime::vec3 m_attach_offset;

            object(sqf::runtime::config config, bool is_vehicle);
            object(const object& obj) = delete;
        public:
            sqf::runtime::config config() const { return m_config; }
            size_t netid() const { return m_netid; }
            bool is_vehicle() const { return m_is_vehicle; }
            bool alive() const { return m_damage < 1; }

            ::sqf::runtime::vec3 position() const { return m_position; }
            void position(::sqf::runtime::vec3 vec) { m_position = vec; }

            ::sqf::runtime::vec3 velocity() const { return m_velocity; }
            void velocity(::sqf::runtime::vec3 vec) { m_velocity = vec; }

            std::string_view varname() const { return m_varname; }
            void varname(std::string str) { m_varname = str; }

            std::shared_ptr<d_group> group() const { return m_group; }
            void group(std::shared_ptr<d_group> g) { m_group = g; }

            float damage() const { return m_damage; }
            void damage(float val) { m_damage = val < 0 ? 0 : val > 1 ? 1 : val; }
            void damage_by(float val) { auto newdmg = m_damage += val; damage(newdmg); }

            /// <summary>
            /// d_object this object is part of (eg. Unit X sits in Vehicle Y, Y will be parent of X)
            /// </summary>
            /// <returns></returns>
            std::shared_ptr<d_object> parent_object() const { return m_parent_object; }

            std::shared_ptr<d_object> driver() const { return m_driver; }
            void driver(std::shared_ptr<d_object> val);

            std::shared_ptr<d_object> gunner() const { return m_gunner; }
            void gunner(std::shared_ptr<d_object> val);

            std::shared_ptr<d_object> commander() const { return m_commander; }
            void commander(std::shared_ptr<d_object> val);

            soldiers_ soldiers() { return m_soldiers; }
            const configuration_ configuration() const { return m_configuration; }

            float direction() const { return m_direction; }
            void direction(float val) { m_direction = val; }

            std::shared_ptr<d_object> attached_to() const { return m_attached_to; }
            ::sqf::runtime::vec3 attach_offset() const { return m_attach_offset; }
            void attach_to(std::shared_ptr<d_object> parent, ::sqf::runtime::vec3 offset) { m_attached_to = std::move(parent); m_attach_offset = offset; }
            void detach() { m_attached_to = {}; }

            bool captive() const { return m_captive; }
            void captive(bool val) { m_captive = val; }

            bool allow_damage() const { return m_allow_damage; }
            void allow_damage(bool val) { m_allow_damage = val; }

            bool ai_disabled(std::string_view feature) const { return m_disabled_ai.find(std::string(feature)) != m_disabled_ai.end(); }
            void disable_ai(std::string feature) { m_disabled_ai.insert(std::move(feature)); }
            void enable_ai(const std::string& feature) { m_disabled_ai.erase(feature); }

            /// <summary>
            /// Stores an addAction entry (its full argument array, exactly
            /// as passed - nothing here ever actually shows or fires it,
            /// there is no display to show it on) and returns a fresh id.
            /// </summary>
            size_t add_action(::sqf::runtime::value action) { auto id = m_next_action_id++; m_actions.push_back({ id, std::move(action) }); return id; }
            void remove_action(size_t id) { auto it = std::find_if(m_actions.begin(), m_actions.end(), [id](const auto& p) { return p.first == id; }); if (it != m_actions.end()) { m_actions.erase(it); } }
            const std::vector<std::pair<size_t, ::sqf::runtime::value>>& actions() const { return m_actions; }

            /// <summary>
            /// setUnitTrait/getUnitTrait storage - a handful of named
            /// values at most per unit, so a linear scan (matching m_actions
            /// above) beats pulling in a map for this.
            /// </summary>
            void set_trait(const std::string& name, ::sqf::runtime::value val)
            {
                auto it = std::find_if(m_traits.begin(), m_traits.end(), [&name](const auto& p) { return p.first == name; });
                if (it != m_traits.end()) { it->second = std::move(val); }
                else { m_traits.push_back({ name, std::move(val) }); }
            }
            std::optional<::sqf::runtime::value> trait(const std::string& name) const
            {
                auto it = std::find_if(m_traits.begin(), m_traits.end(), [&name](const auto& p) { return p.first == name; });
                if (it == m_traits.end()) { return std::nullopt; }
                return it->second;
            }

            /// <summary>
            /// addEventHandler/removeEventHandler storage. Nothing in this
            /// headless engine simulates the real triggers (damage, kills,
            /// GetIn/GetOut, ...) that would fire one, so a handler is
            /// stored well enough to round-trip a real id - the same
            /// minimal-but-real bar add_action above holds to - but is
            /// never actually invoked.
            /// </summary>
            size_t add_event_handler(::sqf::runtime::value handler) { auto id = m_next_eh_id++; m_event_handlers.push_back({ id, std::move(handler) }); return id; }
            void remove_event_handler(size_t id) { auto it = std::find_if(m_event_handlers.begin(), m_event_handlers.end(), [id](const auto& p) { return p.first == id; }); if (it != m_event_handlers.end()) { m_event_handlers.erase(it); } }
            void remove_all_event_handlers() { m_event_handlers.clear(); }


            /// <summary>
            /// Creates a new object instance.
            /// </summary>
            /// <param name="runtime"></param>
            /// <param name="classname"></param>
            /// <param name="isvehicle"></param>
            /// <returns></returns>
            static std::shared_ptr<object> create(::sqf::runtime::runtime& runtime, sqf::runtime::config config, bool is_vehicle);

            /// <summary>
            /// Destroys a given object, invalidating all living d_object instances (making them null).
            /// </summary>
            /// <param name="runtime"></param>
            void destroy(::sqf::runtime::runtime& runtime);

            /// <summary>
            /// Attempts to read the vehicle config entry for this
            /// and sets some local variables according to the config.
            /// </summary>
            /// <returns>
            /// Returns TRUE on success. FALSE will be returned, if
            /// the config class for the vehicle could not be received.
            /// </returns>
            bool update_values_from_confighost(sqf::runtime::confighost host);



            float distance3dsqr(std::shared_ptr<object> obj) const { return distance3dsqr(obj->position()); }
            float distance3dsqr(const object* obj) const { return distance3dsqr(obj->position()); }
            float distance3dsqr(::sqf::runtime::vec3 obj) const { return distance3dsqr(std::array<float, 3>{ obj.x, obj.y, obj.z }); }
            float distance3dsqr(std::array<float, 3> obj) const;
            float distance3d(std::shared_ptr<object> obj) const { return distance3d(obj->position()); }
            float distance3d(const object* obj) const { return distance3d(obj->position()); }
            float distance3d(::sqf::runtime::vec3 obj) const { return distance3d(std::array<float, 3>{ obj.x, obj.y, obj.z }); }
            float distance3d(std::array<float, 3> obj) const;

            float distance2dsqr(std::shared_ptr<object> obj) const { return distance2dsqr(obj->position()); }
            float distance2dsqr(const object* obj) const { return distance2dsqr(obj->position()); }
            float distance2dsqr(::sqf::runtime::vec3 obj) const { return distance2dsqr(std::array<float, 2>{ obj.x, obj.y }); }
            float distance2dsqr(std::array<float, 2> obj) const;
            float distance2d(std::shared_ptr<object> obj) const { return distance2d(obj->position()); }
            float distance2d(const object* obj) const { return distance2d(obj->position()); }
            float distance2d(::sqf::runtime::vec3 obj) const { return distance2d(std::array<float, 2>{ obj.x, obj.y }); }
            float distance2d(std::array<float, 2> obj) const;
        };
    }
}
