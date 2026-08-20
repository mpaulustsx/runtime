#include "ops_object.h"
#include "../runtime/value.h"
#include "../runtime/logging.h"
#include "../runtime/runtime.h"
#include "../runtime/version.h"
#include "../runtime/sqfop.h"
#include "../runtime/util.h"
#include "../runtime/instruction_set.h"
#include "../runtime/d_string.h"
#include "../runtime/d_scalar.h"
#include "../runtime/d_boolean.h"
#include "../runtime/d_array.h"
#include "../runtime/d_code.h"
#include "d_config.h"

#include "dlops_storage.h"


#include "object.h"
#include "d_object.h"
#include "group.h"
#include "d_group.h"
#include "d_side.h"


#include <cstdlib>
#include <algorithm>


namespace err = logmessage::runtime;
using namespace sqf::runtime;
using namespace sqf::types;
using namespace std::string_literals;

namespace
{
    value objnull_(runtime& runtime)
    {
        return value(std::make_shared<d_object>());
    }
    value typeof_object(runtime& runtime, value::cref right)
    {
        auto obj = right.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            runtime.__logmsg(err::ReturningEmptyString(runtime.context_active().current_frame().diag_info_from_position()));
            return "";
        }
        else
        {
            return obj->value()->config().name();
        }
    }
    value createvehicle_array(runtime& runtime, value::cref right)
    {
        auto arr = right.data<d_array>();
        if (!arr->check_type(runtime, std::array<sqf::runtime::type, 5>{ t_string(), t_array(), t_array(), t_string(), }))
        {
            return {};
        }
        auto type = arr->at(0).data<d_string, std::string>();
        auto position = arr->at(1).data<d_array>();
        if (!position->check_type(runtime, t_scalar(), 3))
        {
            return {};
        }
        auto radius = arr->at(3).data<d_scalar, float>();
        config conf;
        if (runtime.configuration().enable_classname_check)
        {
            auto configBin = runtime.confighost().root();

            auto cfgVehicles = configBin / "CfgVehicles";
            if (cfgVehicles.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin" }, "CfgVehicles"));
                return {};
            }

            auto opt = cfgVehicles / type;
            if (opt.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin", "CfgVehicles" }, type));
                return {};
            }
            else
            {
                conf = *opt;
            }
        }
        auto veh = object::create(runtime, conf, true);
        veh->position({
            position->at(0).data<d_scalar, float>() + ((std::rand() % static_cast<int>(radius * 2)) - radius),
            position->at(1).data<d_scalar, float>() + ((std::rand() % static_cast<int>(radius * 2)) - radius),
            position->at(2).data<d_scalar, float>()
            });
        return std::make_shared<d_object>(veh);
    }
    value createvehicle_string_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto type = left.data<d_string, std::string>();
        auto position = right.data<d_array>();
        if (!position->check_type(runtime, t_scalar(), 3))
        {
            return {};
        }
        if (!position->check_type(runtime, t_scalar(), 3))
        {
            return {};
        }
        config conf;
        if (runtime.configuration().enable_classname_check)
        {
            auto configBin = runtime.confighost().root();

            auto cfgVehicles = configBin / "CfgVehicles";
            if (cfgVehicles.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin" }, "CfgVehicles"));
                return {};
            }

            auto opt = cfgVehicles / type;
            if (opt.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin", "CfgVehicles" }, type));
                return {};
            }
            else
            {
                conf = *opt;
            }
        }
        auto veh = object::create(runtime, conf, true);
        veh->position({
            position->at(0).data<d_scalar, float>(),
            position->at(1).data<d_scalar, float>(),
            position->at(2).data<d_scalar, float>()
            });
        return std::make_shared<d_object>(veh);
    }

    value deletevehicle_array(runtime& runtime, value::cref right)
    {
        auto veh = right.data<d_object>();
        if (veh->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        veh->value()->destroy(runtime);
        return {};
    }
    value position_object(runtime& runtime, value::cref right)
    {
        auto veh = right.data<d_object>();
        if (veh->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto pos = veh->value()->position();
        auto arr = std::make_shared<d_array>();
        arr->push_back(pos.x);
        arr->push_back(pos.y);
        arr->push_back(pos.z);
        return value(arr);
    }
    value setpos_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto veh = left.data<d_object>();
        if (veh->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto position = right.data<d_array>();
        position->check_type(runtime, t_scalar(), 3);
        auto inner = veh->value();
        inner->position({
            position->at(0).data<d_scalar, float>(),
            position->at(1).data<d_scalar, float>(),
            position->at(2).data<d_scalar, float>()
            });
        return {};
    }
    value velocity_object(runtime& runtime, value::cref right)
    {
        auto veh = right.data<d_object>();
        if (veh->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto vel = veh->value()->velocity();
        auto arr = std::make_shared<d_array>();
        arr->push_back(vel.x);
        arr->push_back(vel.y);
        arr->push_back(vel.z);
        return value(arr);
    }
    value setvelocity_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto veh = left.data<d_object>();
        if (veh->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto velocity = right.data<d_array>();
        velocity->check_type(runtime, t_scalar(), 3);
        auto inner = veh->value();
        inner->velocity({
            velocity->at(0).data<d_scalar, float>(),
            velocity->at(1).data<d_scalar, float>(),
            velocity->at(2).data<d_scalar, float>()
            });
        return {};
    }
    value domove_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>()->value();
        if (obj->is_vehicle())
        {
            runtime.__logmsg(err::ExpectedUnit(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        setpos_object_array(runtime, left, right);
        return {};
    }
    value domove_array_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto arr = left.data<d_array>();
        bool errflag = false;
        for (size_t i = 0; i < arr->size(); i++)
        {
            if (!arr->at(i).is<t_object>())
            {
                runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), i, t_object(), arr->at(i).type()));
                errflag = true;
            }
            else if (arr->at(i).data<d_object>()->value()->is_vehicle())
            {
                runtime.__logmsg(err::ExpectedUnit(runtime.context_active().current_frame().diag_info_from_position()));
                errflag = true;
            }
        }
        if (errflag)
        {
            return {};
        }
        for (const auto& i : *arr)
        {
            setpos_object_array(runtime, i, right);
        }
        return {};
    }

    value createUnit_group_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto grp = left.data<d_group>();
        auto arr = right.data<d_array>();
        
        if (arr->check_type(runtime, std::array<sqf::runtime::type, 5> { t_string(), t_array(), t_array(), t_scalar(), t_string() }))
        {
            return {};
        }
        auto type = arr->at(0).data<d_string, std::string>();
        auto position = arr->at(1).data<d_array>();
        if (!position->check_type(runtime, t_scalar(), 3))
        {
            return {};
        }
        auto radius = arr->at(3).data<d_scalar, float>();
        config conf;
        if (runtime.configuration().enable_classname_check)
        {
            auto configBin = runtime.confighost().root();

            auto cfgVehicles = configBin / "CfgVehicles";
            if (cfgVehicles.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin" }, "CfgVehicles"));
                return {};
            }

            auto opt = cfgVehicles / type;
            if (opt.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin", "CfgVehicles" }, type));
                return {};
            }
            else
            {
                conf = *opt;
            }
        }
        auto veh = object::create(runtime, conf, false);
        veh->position({
            position->at(0).data<d_scalar, float>() + ((std::rand() % static_cast<int>(radius * 2)) - radius),
            position->at(1).data<d_scalar, float>() + ((std::rand() % static_cast<int>(radius * 2)) - radius),
            position->at(2).data<d_scalar, float>()
            });
        return std::make_shared<d_object>(veh);
    }
    value createUnit_string_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto type = left.data<d_string, std::string>();
        auto arr = right.data<d_array>();
        double skill = 0.5;
        std::string rank = "PRIVATE";

        if (arr->size() < 2)
        {
            runtime.__logmsg(err::ExpectedMinimumArraySizeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 2, arr->size()));
            return {};
        }
        // Position
        if (!arr->at(0).is< t_array>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 0, t_array(), arr->at(0).type()));
            return {};
        }
        auto position = arr->at(0).data<d_array>();
        if (!position->check_type(runtime, t_scalar(), 3))
        {
            return {};
        }
        // Group
        if (!arr->at(1).is<t_group>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 1, t_group(), arr->at(1).type()));
            return {};
        }
        auto grp = arr->at(1).data<d_group>();

        //Optionals
        //init
        if (arr->size() >= 3)
        {
            if (!arr->at(2).is< t_string>())
            {
                runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 2, t_string(), arr->at(2).type()));
                return {};
            }
            else
            {
                std::string init = arr->at(2).data<d_string, std::string>();
            }
        }
        //skill
        if (arr->size() >= 4)
        {
            if (!arr->at(3).is< t_scalar>())
            {
                runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 3, t_scalar(), arr->at(3).type()));
                return {};
            }
            else
            {
                skill = arr->at(3).data<d_scalar, float>();
            }
        }
        //rank
        if (arr->size() >= 5)
        {
            if (!arr->at(4).is<t_string>())
            {
                runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 4, t_string(), arr->at(4).type()));
                return {};
            }
            else
            {
                rank = arr->at(4).data<d_string, std::string>();
            }
        }
        config conf;
        if (runtime.configuration().enable_classname_check)
        {
            auto configBin = runtime.confighost().root();

            auto cfgVehicles = configBin / "CfgVehicles";
            if (cfgVehicles.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin" }, "CfgVehicles"));
                return {};
            }

            auto opt = cfgVehicles / type;
            if (opt.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin", "CfgVehicles" }, type));
                return {};
            }
            else
            {
                conf = *opt;
            }
        }
        auto veh = object::create(runtime, conf, false);
        auto obj = std::make_shared<d_object>(veh);
        if (!grp->is_null())
        {
            grp->value()->push_back(obj);
        }

        veh->position({
            position->at(0).data<d_scalar, float>(),
            position->at(1).data<d_scalar, float>(),
            position->at(2).data<d_scalar, float>()
            });
        return obj;
    }
    value distance_array_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_array>();
        auto r = right.data<d_array>();
        if (!l->check_type(runtime, t_scalar(), 2, 3) || !r->check_type(runtime, t_scalar(), 2, 3))
        {
            return {};
        }
        return distance3d(l, r);
    }
    value distance_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_object>();
        auto r = right.data<d_array>();
        if (l->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (!r->check_type(runtime, t_scalar(), 2, 3))
        {
            return {};
        }
        return l->value()->distance3d(*r);
    }
    value distance_array_object(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_array>();
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (!l->check_type(runtime, t_scalar(), 2, 3))
        {
            return {};
        }
        return r->value()->distance3d(*l);
    }
    value distance_object_object(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_object>();
        auto r = right.data<d_object>();
        if (l->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        return l->value()->distance3d(r->value());
    }
    value distance2d_array_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_array>();
        auto r = right.data<d_array>();
        if (!l->check_type(runtime, t_scalar(), 2, 3) || !r->check_type(runtime, t_scalar(), 2, 3))
        {
            return {};
        }
        return distance2d(l, r);
    }
    value distance2d_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_object>();
        auto r = right.data<d_array>();
        if (l->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (!r->check_type(runtime, t_scalar(), 2, 3))
        {
            return {};
        }
        return l->value()->distance2d(*r);
    }
    value distance2d_array_object(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_array>();
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (!l->check_type(runtime, t_scalar(), 2, 3))
        {
            return {};
        }
        return r->value()->distance2d(*l);
    }
    value distance2d_object_object(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_object>();
        auto r = right.data<d_object>();
        if (l->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        return l->value()->distance2d(r->value());
    }
    class nearestobjects_distancesort3d
    {
        std::array<float, 3> pos;
    public:
        nearestobjects_distancesort3d(std::array<float, 3> p) : pos(p) {}
        nearestobjects_distancesort3d(vec3 p) : pos({ p.x, p.y, p.z }) {}
        bool operator() (value::cref l, value::cref r) const { return l.data<d_object>()->value()->distance3d(pos) < r.data<d_object>()->value()->distance3d(pos); }
    };
    class nearestobjects_distancesort2d
    {
        std::array<float, 2> pos;
    public:
        nearestobjects_distancesort2d(std::array<float, 2> p) : pos(p) {}
        nearestobjects_distancesort2d(vec3 p) : pos({ p.x, p.y }) {}
        bool operator() (value::cref l, value::cref r) const { return l.data<d_object>()->value()->distance2d(pos) < r.data<d_object>()->value()->distance2d(pos); }
    };
    value nearestobjects_array(runtime& runtime, value::cref right)
    {
        auto arr = right.data<d_array>();
        if (arr->size() != 3 && arr->size() != 4)
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 3, 4, arr->size()));
            return {};
        }
        vec3 position {0, 0, 0};
        if (arr->at(0).is<t_array>())
        {
            if (!arr->at(0).data<d_array>()->check_type(runtime, t_scalar(), 3))
            {
                return {};
            }
            position = *arr->at(0).data<d_array>();
        }
        else if (arr->at(0).is<t_object>())
        {
            if (arr->at(0).data<d_object>()->is_null())
            {
                runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
                return {};
            }
            position = arr->at(0).data<d_object>()->value()->position();
        }
        else
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 0, std::array<sqf::runtime::type, 2> { t_array(), t_object() }, arr->at(0).type()));
            return {};
        }
        if (!arr->at(1).is<t_array>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 1, t_array(), arr->at(1).type()));
            return {};
        }
        auto filterarr = arr->at(1).data<d_array>();
        for (size_t i = 0; i < filterarr->size(); i++)
        {
            if (!filterarr->at(i).is< t_string>())
                runtime.__logmsg(err::ExpectedSubArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), std::array<size_t, 2> { 1, i }, t_string(), filterarr->at(i).type()));
            {
                return {};
            }
        }
        if (!arr->at(2).is< t_scalar>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 2, t_scalar(), arr->at(2).type()));
            return {};
        }
        auto radius = arr->at(2).data<d_scalar, float>();
        auto is2ddistance = false;
        if (arr->size() == 4)
        {
            if (!arr->at(3).is<t_boolean>())
            {
                runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 4, t_boolean(), arr->at(3).type()));
                return {};
            }
            is2ddistance = arr->at(3).data<d_boolean, bool>();
        }
        auto outputarr = std::make_shared<d_array>();
        if (is2ddistance)
        {
            std::array<float, 2> position2d{ position.x, position.y };
            for (auto& object : runtime.storage<object::object_storage>())
            {
                if (object->distance2d(position2d) > radius) continue;

                bool match = filterarr->empty() || !runtime.configuration().enable_classname_check;
                if (!match)
                {
                    auto cfgObject = object->config().navigate(runtime.confighost());

                    if (!cfgObject.empty())
                    {
                        auto found = std::find_if(filterarr->begin(), filterarr->end(), [&cfgObject](value::cref value) {
                            return cfgObject.has_inherited_with_name(value.data<d_string, std::string>());
                        });
                        match = found != filterarr->end();
                    }
                }
                if (match)
                {
                    outputarr->push_back(value(std::make_shared<d_object>(object)));
                }
            }
            std::sort(outputarr->begin(), outputarr->end(), nearestobjects_distancesort2d(position2d));
        }
        else
        {
            for (auto& object : runtime.storage<object::object_storage>())
            {
                if (object->distance3d(position) > radius) continue;
                
                bool match = filterarr->empty() || !runtime.configuration().enable_classname_check;
                if (!match)
                {
                    auto cfgObject = object->config().navigate(runtime.confighost());
                    if (!cfgObject.empty())
                    {
                        auto found = std::find_if(filterarr->begin(), filterarr->end(), [&cfgObject](value::cref value) {
                            return cfgObject.has_inherited_with_name(value.data<d_string, std::string>());
                            });
                        match = found != filterarr->end();
                    }
                }
                if (match)
                {
                    outputarr->push_back(value(std::make_shared<d_object>(object)));
                }
            }
            std::sort(outputarr->begin(), outputarr->end(), nearestobjects_distancesort3d(position));
        }
        return value(outputarr);
    }
    value isnull_object(runtime& runtime, value::cref right)
    {
        auto obj = right.data<d_object>();
        return obj->is_null();
    }
    value side_object(runtime& runtime, value::cref right)
    {
        auto obj = right.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return std::make_shared<d_side>(d_side::side::Empty);
        }
        else
        {
            auto grp = obj->value()->group();
            if (grp->is_null())
            {
                runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
                return std::make_shared<d_side>(d_side::side::Empty);
            }
            else
            {
                return grp->value()->side();
            }
        }
    }
    value allunits_(runtime& runtime)
    {
        auto arr = std::make_shared<d_array>();
        for (auto& object : runtime.storage<object::object_storage>())
        {
            if (object->is_vehicle())
                continue;
            arr->push_back(value(std::make_shared<d_object>(object)));
        }
        return value(arr);
    }
    value iskindof_object_string(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto base_type_str = right.data<d_string, std::string>();
        return obj->value()->config().navigate(runtime.confighost()).has_inherited_with_name(base_type_str);
    }
    value iskindof_string_string(runtime& runtime, value::cref left, value::cref right)
    {
        auto test_type_str = left.data<d_string, std::string>();
        auto base_type_str = right.data<d_string, std::string>();
        {
            auto configBin = runtime.confighost().root();

            auto cfgVehicles = configBin / "CfgVehicles";
            if (cfgVehicles.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin" }, "CfgVehicles"));
                return {};
            }

            auto test_opt = cfgVehicles / test_type_str;
            if (test_opt.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin", "CfgVehicles" }, test_type_str));
                return {};
            }

            return test_opt.has_inherited_with_name(base_type_str);
        }
    }
    value iskindof_string_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto test_type_str = left.data<d_string, std::string>();
        auto arr = right.data<d_array>();
        if (!arr->check_type(runtime, std::array<sqf::runtime::type, 2>{ t_string(), t_config() }))
        {
            return {};
        }
        auto base_type_str = arr->at(0).data<d_string, std::string>();
        auto base_conf = arr->at(1).data<d_config, config>();

        {
            auto nav = base_conf.navigate(runtime.confighost());
            auto test_opt = nav / test_type_str;
            if (test_opt.empty())
            {
                // Get navigation path
                std::vector<std::string> path;
                do
                {
                    path.push_back(nav->name);
                    nav = nav.parent_logical();
                } while (nav->id_parent_logical != config::invalid_id);
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), path, test_type_str));
                return {};
            }

            return test_opt.has_inherited_with_name(base_type_str);
        }
    }
    value player_(runtime& runtime)
    {
        return std::make_shared<d_object>(runtime.storage<object::object_storage>().player());
    }

    value setdamage_object_scalar(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_object>();
        auto r = right.data<d_scalar, float>();
        if (l->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        l->value()->damage(r);
        return {};
    }
    value getdamage_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        return r->value()->damage();
    }
    value alive_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return false;
        }
        return r->value()->alive();
    }
    value crew_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto arr = std::make_shared<d_array>();
        auto obj = r->value();
        if (!obj->is_vehicle())
        {
            runtime.__logmsg(err::ExpectedVehicleWeak(runtime.context_active().current_frame().diag_info_from_position()));
            runtime.__logmsg(err::ReturningEmptyArray(runtime.context_active().current_frame().diag_info_from_position()));
            return value(arr);
        }
        if (!obj->driver()->is_null())
        {
            arr->push_back(value(obj->driver()));
        }
        if (!obj->gunner()->is_null())
        {
            arr->push_back(value(obj->gunner()));
        }
        if (!obj->commander()->is_null())
        {
            arr->push_back(value(obj->commander()));
        }
        for (auto& it : obj->soldiers())
        {
            arr->push_back(value(it));
        }
        return value(arr);
    }
    value vehicle_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto obj = r->value();
        if (!obj->is_vehicle())
        {
            runtime.__logmsg(err::ExpectedVehicleWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return right;
        }
        auto parent = obj->parent_object();
        if (parent->is_null() || !parent->value()->is_vehicle())
        {
            return right;
        }
        else
        {
            return value(parent);
        }
    }
    value objectparent_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        return value(r->value()->parent_object());
    }
    value driver_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto obj = r->value();
        if (!obj->is_vehicle())
        {
            runtime.__logmsg(err::ExpectedVehicleWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return right;
        }
        return value(obj->driver());
    }
    value commander_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto obj = r->value();
        if (!obj->is_vehicle())
        {
            runtime.__logmsg(err::ExpectedVehicleWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return right;
        }
        return value(obj->commander());
    }
    value gunner_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto obj = r->value();
        if (!obj->is_vehicle())
        {
            runtime.__logmsg(err::ExpectedVehicleWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return right;
        }
        return value(obj->gunner());
    }
    value in_object_object(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_object>();
        if (l->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (l->value()->is_vehicle())
        {
            runtime.__logmsg(err::ExpectedUnitWeak(runtime.context_active().current_frame().diag_info_from_position()));
            runtime.__logmsg(err::ReturningFalse(runtime.context_active().current_frame().diag_info_from_position()));
            return false;
        }
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (!r->value()->is_vehicle())
        {
            runtime.__logmsg(err::ExpectedUnitWeak(runtime.context_active().current_frame().diag_info_from_position()));
            runtime.__logmsg(err::ReturningFalse(runtime.context_active().current_frame().diag_info_from_position()));
            return false;
        }
        auto veh = r->value();
        auto unit = l->value();
        if (veh->driver()->value().get() == unit.get() || veh->commander()->value().get() == unit.get() || veh->gunner()->value().get() == unit.get())
        {
            return true;
        }
        else
        {
            auto res = std::find_if(veh->soldiers().begin(), veh->soldiers().end(), [unit](std::shared_ptr<d_object> data) -> bool {
                return data->value().get() == unit.get();
            });
            return res != veh->soldiers().end();
        }
    }
    value vehiclevarname_object(runtime& runtime, value::cref right)
    {
        auto r = right.data<d_object>();
        if (r->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        return r->value()->varname();
    }
    value setvehiclevarname_object_string(runtime& runtime, value::cref left, value::cref right)
    {
        auto l = left.data<d_object>();
        if (l->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto r = right.data<d_string, std::string>();
        l->value()->varname(r);
        return {};
    }
    value allvariables_object(runtime& runtime, value::cref right)
    {
        auto obj = right.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            runtime.__logmsg(err::ReturningEmptyArray(runtime.context_active().current_frame().diag_info_from_position()));
            return std::make_shared<d_array>();
        }
        auto scope = std::static_pointer_cast<value_scope>(obj->value());

        std::vector<value> variable_names;

        for (auto& kvp : *scope)
        {
            variable_names.push_back(kvp.first);
        }
        return variable_names;
    }
    value getVariable_object_string(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            runtime.__logmsg(err::ReturningNil(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto scope = std::static_pointer_cast<value_scope>(obj->value());
        auto variable = right.data<d_string, std::string>();

        auto res = scope->try_get(variable);
        if (res.has_value())
        {
            return *res;
        }
        return {};
    }
    value getVariable_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            runtime.__logmsg(err::ReturningNil(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto scope = std::static_pointer_cast<value_scope>(obj->value());
        auto r = right.data<d_array>();
        if (r->size() != 2)
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 2, r->size()));
            runtime.__logmsg(err::ReturningNil(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        if (!r->at(0).is<t_string>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 2, t_string(), r->at(0).type()));
            runtime.__logmsg(err::ReturningNil(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }

        auto res = scope->try_get(r->at(0).data<d_string, std::string>());
        if (res.has_value())
        {
            return *res;
        }
        return r->at(1);
    }
    value setVariable_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto scope = std::static_pointer_cast<value_scope>(obj->value());
        auto r = right.data<d_array>();
        // A third element is accepted the way real Arma accepts an
        // isPublic broadcast flag here: there is no network to broadcast
        // over in a single process, so it is read and otherwise ignored
        // rather than rejected outright.
        if (r->size() != 2 && r->size() != 3)
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 2, r->size()));
            return {};
        }
        if (!r->at(0).is<t_string>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 2, t_string(), r->at(0).type()));
            return {};
        }

        scope->at(r->at(0).data<d_string, std::string>()) = r->at(1);
        return {};
    }
    value units_object(runtime& runtime, value::cref right)
    {
        auto obj = right.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            runtime.__logmsg(err::ReturningEmptyString(runtime.context_active().current_frame().diag_info_from_position()));
            return std::make_shared<d_array>();
        }
        else
        {
            auto grp = obj->value()->group();
            if (grp->is_null())
            {
                runtime.__logmsg(err::ExpectedNonNullValue(runtime.context_active().current_frame().diag_info_from_position()));
                runtime.__logmsg(err::ReturningEmptyString(runtime.context_active().current_frame().diag_info_from_position()));
                return std::make_shared<d_array>();
            }
            else
            {
                return std::vector<value>(grp->value()->begin(), grp->value()->end());
            }
        }
    }

    // = = = = = = = = Graduated from ops_dummy_*.cpp = = = = = = = =
    // Headless-appropriate real implementations for commands this fork used
    // to only warn-and-noop on. Each keeps the same minimal-but-real bar the
    // rest of this file holds to: no rendering, no physics, no real AI or
    // network - just state a script can set and read back faithfully.

    value setdir_object_scalar(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        obj->value()->direction(right.data<d_scalar, float>());
        return {};
    }

    value setcaptive_object_boolean(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        obj->value()->captive(right.data<d_boolean, bool>());
        return {};
    }

    value allowdamage_object_boolean(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        obj->value()->allow_damage(right.data<d_boolean, bool>());
        return {};
    }

    value disableai_object_string(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        obj->value()->disable_ai(right.data<d_string, std::string>());
        return {};
    }

    value worldsize_(runtime& runtime)
    {
        // No terrain is loaded in this headless engine. Vindicta's own
        // mission folder (Vindicta.Altis) targets Altis, so that is the
        // size a script asking "how big is the map" should see.
        return 15360.0f;
    }

    value nearroads_array_scalar(runtime& runtime, value::cref left, value::cref right)
    {
        auto pos = left.data<d_array>();
        if (!pos->check_type(runtime, t_scalar(), 2, 3))
        {
            return {};
        }
        // There is no road network modeled in this headless engine - an
        // empty result is the truthful answer, not a placeholder.
        return std::make_shared<d_array>();
    }

    value nearroads_object_scalar(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        return std::make_shared<d_array>();
    }

    value addaction_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto arr = right.data<d_array>();
        if (arr->size() < 2)
        {
            runtime.__logmsg(err::ExpectedMinimumArraySizeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 2, arr->size()));
            return {};
        }
        // Nothing in this headless engine ever shows or fires this action -
        // there is no display to show it on - so it is only stored well
        // enough for addAction/removeAction to round-trip a real id, which
        // is what scripted code actually depends on.
        auto id = obj->value()->add_action(right);
        return static_cast<float>(id);
    }

    value removeaction_object_scalar(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto id = right.data<d_scalar, float>();
        obj->value()->remove_action(static_cast<size_t>(id));
        return {};
    }

    value createagent_array(runtime& runtime, value::cref right)
    {
        auto arr = right.data<d_array>();
        if (!arr->check_type(runtime, std::array<sqf::runtime::type, 5>{ t_string(), t_array(), t_array(), t_scalar(), t_string() }))
        {
            return {};
        }
        auto type = arr->at(0).data<d_string, std::string>();
        auto position = arr->at(1).data<d_array>();
        if (!position->check_type(runtime, t_scalar(), 2, 3))
        {
            return {};
        }
        auto radius = arr->at(3).data<d_scalar, float>();
        config conf;
        if (runtime.configuration().enable_classname_check)
        {
            auto configBin = runtime.confighost().root();

            auto cfgVehicles = configBin / "CfgVehicles";
            if (cfgVehicles.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin" }, "CfgVehicles"));
                return {};
            }

            auto opt = cfgVehicles / type;
            if (opt.empty())
            {
                runtime.__logmsg(err::ConfigEntryNotFoundWeak(runtime.context_active().current_frame().diag_info_from_position(), std::array<std::string, 2> { "ConfigBin", "CfgVehicles" }, type));
                return {};
            }
            else
            {
                conf = *opt;
            }
        }
        auto agent = object::create(runtime, conf, false);
        // Guard against radius == 0 - std::rand() % 0 is undefined behavior,
        // and createAgent 0 (an exact spawn point, no scatter) is the
        // common case for a mod placing a specific NPC.
        float offX = radius > 0 ? static_cast<float>((std::rand() % static_cast<int>(radius * 2)) - radius) : 0.0f;
        float offY = radius > 0 ? static_cast<float>((std::rand() % static_cast<int>(radius * 2)) - radius) : 0.0f;
        agent->position({
            position->at(0).data<d_scalar, float>() + offX,
            position->at(1).data<d_scalar, float>() + offY,
            position->size() > 2 ? position->at(2).data<d_scalar, float>() : 0.0f
            });
        return std::make_shared<d_object>(agent);
    }

    // Shared by both the binary form (args left, ["funcName", target, jip]
    // right) and the unary form (["funcName", target, jip] right only,
    // _this defaults to []) - real remoteExec/remoteExecCall support both.
    value remoteexec_impl(runtime& runtime, value::cref this_, value::cref right)
    {
        auto r = right.data<d_array>();
        if (r->size() < 1)
        {
            runtime.__logmsg(err::ExpectedMinimumArraySizeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 1, r->size()));
            return {};
        }
        if (!r->at(0).is<t_string>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatch(runtime.context_active().current_frame().diag_info_from_position(), 0, t_string(), r->at(0).type()));
            return {};
        }
        // Target machine and JIP (r->at(1)/r->at(2)) are read implicitly by
        // being ignored: there is no network to route them over in a
        // single-process VM, so the named function just runs here, the same
        // simplification setVariable's isPublic flag already makes.
        auto funcname = r->at(0).data<d_string, std::string>();
        auto scope = runtime.default_value_scope();
        auto func = scope->try_get(funcname);
        if (!func.has_value() || !func->is<t_code>())
        {
            // Unknown or non-code function name - nothing to run, same as
            // real remoteExec silently doing nothing for a target with no
            // matching JIP data (and the empty-funcname JIP-cancel idiom
            // some scripted code uses).
            return {};
        }
        frame f = { runtime.default_value_scope(), func->data<d_code, instruction_set>() };
        f["_this"] = this_;
        runtime.context_active().push_frame(f);
        return {};
    }

    value remoteexec_array_array(runtime& runtime, value::cref left, value::cref right)
    {
        return remoteexec_impl(runtime, left, right);
    }

    value remoteexec_array(runtime& runtime, value::cref right)
    {
        return remoteexec_impl(runtime, std::make_shared<d_array>(), right);
    }

    // = = = = = Second batch, graduated from ops_dummy_*.cpp = = = = =
    // Same minimal-but-real bar as the batch above.

    value getplayeruid_object(runtime& runtime, value::cref right)
    {
        auto obj = right.data<d_object>();
        if (obj->is_null())
        {
            return std::string("");
        }
        // Real Arma's UID is a stable Steam/BattlEye identity string, none
        // of which exists in this single-process headless engine. A
        // non-player object always gets "" (the real, documented
        // behavior); the one modeled player gets a fixed, deterministic
        // id so scripted code that stores/looks up "the player's UID" sees
        // the same value on every call within a run.
        auto& storage = runtime.storage<object::object_storage>();
        if (obj->value() == storage.player())
        {
            return std::string("sqfvm-headless-player");
        }
        return std::string("");
    }

    value owner_object(runtime& runtime, value::cref right)
    {
        auto obj = right.data<d_object>();
        if (obj->is_null())
        {
            return 0.0f;
        }
        // No network in this single-process VM - every object is
        // effectively owned by the one machine running it. 2 matches the
        // "server/creator" convention scripted code in this project's own
        // codebase already checks for (e.g. PlayerDatabaseServer.sqf).
        return 2.0f;
    }

    value removeeventhandler_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        // Soft/no-throw validation throughout (Weak log variants, manual
        // checks instead of d_array::check_type's strong ones) rather than
        // the hard array-contract check addEventHandler below still uses -
        // this fork's own CBA_fnc_addBISEventHandler callers (undefined
        // under SQF-VM, so every id they'd have stored is nil) end up
        // calling this with a malformed id already; removing nothing is
        // the same safe no-op remove_event_handler already gives a
        // not-found id, matching removeAction's precedent above.
        auto arr = right.data<d_array>();
        if (arr->size() < 2)
        {
            runtime.__logmsg(err::ExpectedMinimumArraySizeMissmatchWeak(runtime.context_active().current_frame().diag_info_from_position(), 2, arr->size()));
            return {};
        }
        if (!arr->at(1).is<t_scalar>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatchWeak(runtime.context_active().current_frame().diag_info_from_position(), 1, t_scalar(), arr->at(1).type()));
            return {};
        }
        auto id = arr->at(1).data<d_scalar, float>();
        obj->value()->remove_event_handler(static_cast<size_t>(id));
        return {};
    }

    value addeventhandler_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto arr = right.data<d_array>();
        if (arr->size() < 2 || !arr->at(0).is<t_string>() || !arr->at(1).is<t_code>())
        {
            runtime.__logmsg(err::ExpectedArraySizeMissmatchWeak(runtime.context_active().current_frame().diag_info_from_position(), 2, 2, arr->size()));
            return {};
        }
        // Stored well enough to round-trip a real id (removeEventHandler/
        // removeAllEventHandlers below both depend on that), same as
        // addAction above - nothing in this headless engine simulates the
        // real triggers (damage, kills, GetIn/GetOut, ...) that would ever
        // actually fire one.
        auto id = obj->value()->add_event_handler(right);
        return static_cast<float>(id);
    }

    value removealleventhandlers_object_string(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        // Real removeAllEventHandlers only clears handlers of the named
        // type; this fork doesn't track a type per handler (see
        // add_event_handler), so it clears all of them - the only
        // observable difference is that a later removeEventHandler call
        // for an unrelated type now has nothing left to remove either,
        // which is already a safe no-op.
        obj->value()->remove_all_event_handlers();
        return {};
    }

    value isplayer_object(runtime& runtime, value::cref right)
    {
        auto obj = right.data<d_object>();
        if (obj->is_null())
        {
            return false;
        }
        auto& storage = runtime.storage<object::object_storage>();
        return obj->value() == storage.player();
    }

    value setunittrait_object_array(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            runtime.__logmsg(err::ExpectedNonNullValueWeak(runtime.context_active().current_frame().diag_info_from_position()));
            return {};
        }
        auto arr = right.data<d_array>();
        if (arr->size() < 2)
        {
            runtime.__logmsg(err::ExpectedMinimumArraySizeMissmatchWeak(runtime.context_active().current_frame().diag_info_from_position(), 2, arr->size()));
            return {};
        }
        if (!arr->at(0).is<t_string>())
        {
            runtime.__logmsg(err::ExpectedArrayTypeMissmatchWeak(runtime.context_active().current_frame().diag_info_from_position(), 0, t_string(), arr->at(0).type()));
            return {};
        }
        auto name = arr->at(0).data<d_string, std::string>();
        obj->value()->set_trait(name, arr->at(1));
        return {};
    }

    value getunittrait_object_string(runtime& runtime, value::cref left, value::cref right)
    {
        auto obj = left.data<d_object>();
        if (obj->is_null())
        {
            return {};
        }
        auto name = right.data<d_string, std::string>();
        auto val = obj->value()->trait(name);
        if (!val.has_value())
        {
            return {};
        }
        return *val;
    }

    // remoteExecCall runs the same way remoteExec does above - the only
    // real-Arma difference (bypassing the CfgRemoteExec allow-list) has no
    // equivalent to bypass here, since nothing in this VM enforces one.
    value localize_string(runtime& runtime, value::cref right)
    {
        // A real stringtable lookup needs mission-relative XML parsing this
        // fork doesn't do. Returning the key itself - a genuine, common
        // fallback real engines use for a string that can't be resolved -
        // at least guarantees callers get a real STRING back instead of
        // nil, which is what code doing e.g. `format [localize "STR_X", ...]`
        // actually depends on to not fail its own type check.
        return right.data<d_string, std::string>();
    }

    value nearestlocations_array(runtime& runtime, value::cref right)
    {
        // No terrain/location data is loaded in this headless engine (see
        // nearRoads above for the same situation) - an empty result is the
        // truthful answer, not a placeholder.
        return std::make_shared<d_array>();
    }

    value backpack_object(runtime& runtime, value::cref right)
    {
        // No gear/inventory is simulated in this headless engine - "" (the
        // real, documented return for an empty slot) is the truthful
        // answer for every object here, not a placeholder.
        return std::string("");
    }

    value date_(runtime& runtime)
    {
        // No mission calendar is modeled in this headless engine. A fixed,
        // deterministic value beats nil - scripted code that just wants a
        // real 5-element date array to read fields off of gets one.
        return std::make_shared<d_array>(std::vector<value>{ 2024.0f, 1.0f, 1.0f, 12.0f, 0.0f });
    }

    value daytime_(runtime& runtime)
    {
        return 12.0f;
    }

    value publicvariable_string(runtime& runtime, value::cref right)
    {
        // No network in this single-process VM - there's nothing to
        // broadcast to, the same simplification setVariable's isPublic
        // flag and remoteExec's target argument already make elsewhere in
        // this file.
        return {};
    }
}
void sqf::operators::ops_object(sqf::runtime::runtime& runtime)
{
    using namespace sqf::runtime::sqfop;

    runtime.register_sqfop(unary("units", t_object(), "Returns an array with all the units in the group of the unit. For a destroyed object an empty array is returned.", units_object));
    runtime.register_sqfop(nular("allUnits", "Return a list of all units (all persons except agents) outside and inside vehicles.", allunits_));
    runtime.register_sqfop(nular("objNull", "A non-existent Object. To compare non-existent objects use isNull or isEqualTo.", objnull_));
    runtime.register_sqfop(unary("typeOf", t_object(), "Returns the config class name of given object.", typeof_object));
    runtime.register_sqfop(unary("createVehicle", t_array(), "Creates an empty object of given classname type.", createvehicle_array));
    runtime.register_sqfop(binary(4, "createVehicle", t_string(), t_array(), "Creates an empty object of given classname type.", createvehicle_string_array));
    runtime.register_sqfop(binary(4, "createVehicleLocal", t_any(), t_any(), "Creates an empty object of given classname type.", createvehicle_string_array));
    runtime.register_sqfop(unary("deleteVehicle", t_object(), "Deletes an object.", deletevehicle_array));
    runtime.register_sqfop(unary("position", t_object(), "Returns the object position in format PositionAGLS. Z value is height over the surface underneath.", position_object));
    runtime.register_sqfop(unary("getPos", t_object(), "Returns the object position in format PositionAGLS. Z value is height over the surface underneath.", position_object));
    runtime.register_sqfop(binary(4, "setPos", t_object(), t_array(), "Sets object position.", setpos_object_array));
    runtime.register_sqfop(unary("velocity", t_object(), "Return velocity (speed vector) of Unit as an array with format [x, y, z].", velocity_object));
    runtime.register_sqfop(binary(4, "setVelocity", t_object(), t_array(), "Set velocity (speed vector) of a vehicle. Units are in metres per second.", setvelocity_object_array));
    runtime.register_sqfop(binary(4, "doMove", t_object(), t_array(), "Order the given unit(s) to move to the given position (without radio messages). In SQFVM this command acts like setPos.", domove_object_array));
    runtime.register_sqfop(binary(4, "doMove", t_array(), t_array(), "Order the given unit(s) to move to the given position (without radio messages). In SQFVM this command acts like setPos.", domove_array_array));
    runtime.register_sqfop(binary(4, "createUnit", t_group(), t_array(), "Create unit of a class that's defined in CfgVehicles.", createUnit_group_array));
    runtime.register_sqfop(binary(4, "createUnit", t_string(), t_array(), "Create unit of a class that's defined in CfgVehicles.", createUnit_string_array));
    runtime.register_sqfop(binary(4, "distance", t_array(), t_array(), "Returns a distance in meters between two positions.", distance_array_array));
    runtime.register_sqfop(binary(4, "distance", t_object(), t_array(), "Returns a distance in meters between two positions.", distance_object_array));
    runtime.register_sqfop(binary(4, "distance", t_array(), t_object(), "Returns a distance in meters between two positions.", distance_array_object));
    runtime.register_sqfop(binary(4, "distance", t_object(), t_object(), "Returns a distance in meters between two positions.", distance_object_object));
    runtime.register_sqfop(binary(4, "distance2d", t_array(), t_array(), "Returns a 2d distance in meters between two positions.", distance2d_array_array));
    runtime.register_sqfop(binary(4, "distance2d", t_object(), t_array(), "Returns a 2d distance in meters between two positions.", distance2d_object_array));
    runtime.register_sqfop(binary(4, "distance2d", t_array(), t_object(), "Returns a 2d distance in meters between two positions.", distance2d_array_object));
    runtime.register_sqfop(binary(4, "distance2d", t_object(), t_object(), "Returns a 2d distance in meters between two positions.", distance2d_object_object));
    runtime.register_sqfop(unary("nearestObjects", t_array(), "Returns a list of nearest objects of the given types to the given position or object, within the specified distance. If more than one object is found they will be ordered by proximity, the closest one will be first in the array.", nearestobjects_array));
    runtime.register_sqfop(unary("isNull", t_object(), "Checks whether the tested item is Null.", isnull_object));
    runtime.register_sqfop(unary("side", t_object(), "Returns the side of an object.", side_object));
    runtime.register_sqfop(binary(4, "isKindOf", t_object(), t_string(), "Checks whether the object is (a subtype) of the given type.", iskindof_object_string));
    runtime.register_sqfop(binary(4, "isKindOf", t_string(), t_string(), "Checks whether the object is (a subtype) of the given type. Checks CfgVehicles, CfgAmmo and CfgNonAiVehicles.", iskindof_string_string));
    runtime.register_sqfop(binary(4, "isKindOf", t_string(), t_array(), "Checks whether the object is (a subtype) of the given type.", iskindof_string_array));
    runtime.register_sqfop(nular("player", "Theoretical player object. Practically, just a normal object.", player_));
    runtime.register_sqfop(binary(4, "setDamage", t_object(), t_scalar(), "Damage / repair object. Damage 0 means fully functional, damage 1 means completely destroyed / dead.", setdamage_object_scalar));
    runtime.register_sqfop(unary("getDammage", t_object(), "Return the damage value of an object.", getdamage_object));
    runtime.register_sqfop(unary("damage", t_object(), "Return the damage value of an object.", getdamage_object));
    runtime.register_sqfop(unary("alive", t_object(), "Check if given vehicle/person/building is alive (i.e. not dead or destroyed). alive objNull returns false.", alive_object));
    runtime.register_sqfop(unary("crew", t_object(), "Returns the crew (both dead and alive) of the given vehicle.", crew_object));
    runtime.register_sqfop(unary("vehicle", t_object(), "Vehicle in which given unit is mounted. If none, unit is returned.", vehicle_object));
    runtime.register_sqfop(unary("objectParent", t_object(), "Returns parent of an object if the object is proxy, otherwise objNull.", objectparent_object));
    runtime.register_sqfop(unary("driver", t_object(), "Returns the driver of a vehicle. If provided object is a unit, the unit is returned.", driver_object));
    runtime.register_sqfop(unary("commander", t_object(), "Returns the primary observer. If provided object is a unit, the unit is returned.", commander_object));
    runtime.register_sqfop(unary("gunner", t_object(), "Returns the gunner of a vehicle. If provided object is a unit, the unit is returned.", gunner_object));
    runtime.register_sqfop(binary(4, "in", t_object(), t_object(), "Checks whether unit is in vehicle.", in_object_object));
    runtime.register_sqfop(unary("vehicleVarName", t_object(), "Returns the name of the variable which contains a primary editor reference to this object." "\n"
        "This is the variable given in the Insert Unit dialog / name field, in the editor. It can be changed using setVehicleVarName.", vehiclevarname_object));
    runtime.register_sqfop(binary(4, "setVehicleVarName", t_object(), t_string(), "Sets string representation of an object to a custom string. For example it is possible to return \"MyFerrari\" instead of default \"ce06b00# 164274: offroad_01_unarmed_f.p3d\" when querying object as string", setvehiclevarname_object_string));

    runtime.register_sqfop(unary("allVariables", t_object(), "Returns a list of all variables from desired namespace.", allvariables_object));
    runtime.register_sqfop(binary(4, "getVariable", t_object(), t_string(), "Return the value of variable in the variable space assigned to various data types. Returns nil if variable is undefined.", getVariable_object_string));
    runtime.register_sqfop(binary(4, "getVariable", t_object(), t_array(), "Return the value of variable in the provided variable space. First element is expected to be the variable name as string. Returns second array item if variable is undefined.", getVariable_object_array));
    runtime.register_sqfop(binary(4, "setVariable", t_object(), t_array(), "Sets a variable to given value in the provided variable space. First element is expected to be the variable name as string. Second element is expected to be anything.", setVariable_object_array));

    // Graduated from ops_dummy_*.cpp - see the function definitions above.
    runtime.register_sqfop(binary(4, "setDir", t_object(), t_scalar(), "Sets the direction in which the vehicle/person is facing.", setdir_object_scalar));
    runtime.register_sqfop(binary(4, "setPosATL", t_object(), t_array(), "Sets object position. No terrain is loaded in this headless engine, so ATL/AGL/ASL all mean the same thing setPos already does.", setpos_object_array));
    runtime.register_sqfop(binary(4, "setCaptive", t_object(), t_boolean(), "Sets whether the object is captive (won't be attacked by AI unless it fires back).", setcaptive_object_boolean));
    runtime.register_sqfop(binary(4, "allowDamage", t_object(), t_boolean(), "Enables/disables damage handling for a unit.", allowdamage_object_boolean));
    runtime.register_sqfop(binary(4, "disableAI", t_object(), t_string(), "Disables the specified AI feature.", disableai_object_string));
    runtime.register_sqfop(nular("worldSize", "Returns the size of the world (map) in meters.", worldsize_));
    runtime.register_sqfop(binary(4, "nearRoads", t_array(), t_scalar(), "Returns objects representing road parts. No road network is modeled in this headless engine, so this always returns an empty array.", nearroads_array_scalar));
    runtime.register_sqfop(binary(4, "nearRoads", t_object(), t_scalar(), "Returns objects representing road parts. No road network is modeled in this headless engine, so this always returns an empty array.", nearroads_object_scalar));
    runtime.register_sqfop(binary(4, "addAction", t_object(), t_array(), "Adds an item to the (script) action menu of the given object. There is no display in this headless engine to ever show it - this only stores it well enough for addAction/removeAction to round-trip a real id.", addaction_object_array));
    runtime.register_sqfop(binary(4, "removeAction", t_object(), t_scalar(), "Removes an action added by addAction.", removeaction_object_scalar));
    runtime.register_sqfop(unary("createAgent", t_array(), "Creates a lightweight unit (no group, minimal AI) of the given classname type.", createagent_array));
    runtime.register_sqfop(binary(4, "remoteExec", t_array(), t_array(), "Executes a function on the specified target machine(s). There is no network in this single-process VM, so the named function is simply run locally; the target and isJIP arguments are accepted but ignored.", remoteexec_array_array));
    runtime.register_sqfop(binary(4, "remoteExecCall", t_array(), t_array(), "Executes a function on the specified target machine(s). Behaves the same as remoteExec in this single-process VM.", remoteexec_array_array));
    runtime.register_sqfop(unary("remoteExec", t_array(), "Executes a function on the specified target machine(s) with no arguments (_this = []). There is no network in this single-process VM, so the named function is simply run locally.", remoteexec_array));
    runtime.register_sqfop(unary("remoteExecCall", t_array(), "Executes a function on the specified target machine(s) with no arguments (_this = []). Behaves the same as remoteExec in this single-process VM.", remoteexec_array));

    // Second batch, graduated from ops_dummy_*.cpp - see the function definitions above.
    runtime.register_sqfop(unary("getPlayerUID", t_object(), "Returns the UID of the given object, if it is the player. There is only ever one player object in this headless engine.", getplayeruid_object));
    runtime.register_sqfop(unary("owner", t_object(), "Returns the network ID of the machine that owns the given object.", owner_object));
    runtime.register_sqfop(binary(4, "removeEventHandler", t_object(), t_array(), "Removes an event handler added by addEventHandler.", removeeventhandler_object_array));
    runtime.register_sqfop(binary(4, "addEventHandler", t_object(), t_array(), "Adds an event handler to the given object. No triggers are ever simulated in this headless engine, so a registered handler is stored well enough to round-trip a real id but never actually invoked.", addeventhandler_object_array));
    runtime.register_sqfop(binary(4, "removeAllEventHandlers", t_object(), t_string(), "Removes all event handlers of the given type from the given object.", removealleventhandlers_object_string));
    runtime.register_sqfop(unary("isPlayer", t_object(), "Checks whether the given object is the (theoretical) player object.", isplayer_object));
    runtime.register_sqfop(binary(4, "setUnitTrait", t_object(), t_array(), "Sets a named unit trait to a given value. First element is expected to be the trait name as string, second element the value.", setunittrait_object_array));
    runtime.register_sqfop(binary(4, "getUnitTrait", t_object(), t_string(), "Returns the value of a named unit trait, or nil if it was never set.", getunittrait_object_string));
    runtime.register_sqfop(unary("localize", t_string(), "Returns the localized string of the given stringtable key. No stringtable is loaded in this headless engine, so the key itself is returned.", localize_string));
    runtime.register_sqfop(unary("nearestLocations", t_array(), "Returns a list of locations near the given position of the given type(s). No terrain/location data is loaded in this headless engine, so this always returns an empty array.", nearestlocations_array));
    runtime.register_sqfop(unary("backpack", t_object(), "Returns the classname of the object's backpack, or \"\" if it has none. No gear/inventory is simulated in this headless engine.", backpack_object));
    runtime.register_sqfop(unary("goggles", t_object(), "Returns the classname of the unit's goggles, or \"\" if it has none. No gear/inventory is simulated in this headless engine.", backpack_object));
    runtime.register_sqfop(unary("headgear", t_object(), "Returns the classname of the unit's headgear, or \"\" if it has none. No gear/inventory is simulated in this headless engine.", backpack_object));
    runtime.register_sqfop(unary("hmd", t_object(), "Returns the classname of the unit's head-mounted display, or \"\" if it has none. No gear/inventory is simulated in this headless engine.", backpack_object));
    runtime.register_sqfop(unary("primaryWeapon", t_object(), "Returns the classname of the unit's primary weapon, or \"\" if it has none. No gear/inventory is simulated in this headless engine.", backpack_object));
    runtime.register_sqfop(unary("secondaryWeapon", t_object(), "Returns the classname of the unit's secondary weapon, or \"\" if it has none. No gear/inventory is simulated in this headless engine.", backpack_object));
    runtime.register_sqfop(unary("uniform", t_object(), "Returns the classname of the unit's uniform, or \"\" if it has none. No gear/inventory is simulated in this headless engine.", backpack_object));
    runtime.register_sqfop(unary("vest", t_object(), "Returns the classname of the unit's vest, or \"\" if it has none. No gear/inventory is simulated in this headless engine.", backpack_object));
    runtime.register_sqfop(unary("getPosATL", t_object(), "Returns the object position in format PositionATL. No terrain is loaded in this headless engine, so ATL/AGL/ASL all mean the same thing position already returns.", position_object));
    runtime.register_sqfop(nular("date", "Returns the current in-game date as an array [year, month, day, hour, minute]. No mission calendar is modeled in this headless engine, so a fixed date is returned.", date_));
    runtime.register_sqfop(nular("daytime", "Returns the current in-game time of day as a decimal number of hours. No mission calendar is modeled in this headless engine, so a fixed value is returned.", daytime_));
    runtime.register_sqfop(unary("publicVariable", t_string(), "Broadcasts a variable to all machines. No network exists in this single-process VM, so this is a no-op.", publicvariable_string));
}