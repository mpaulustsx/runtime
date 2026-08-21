#pragma once
#include "../runtime/instruction.h"
#include "../runtime/value.h"
#include "../runtime/data.h"
#include "../runtime/runtime.h"

#include <string>
#include <string_view>

namespace sqf::opcodes
{
    class assign_to : public sqf::runtime::instruction
    {
    private:
#if _DEBUG
        std::string ___TYPE = "assign_to";
#endif
        std::string m_variable_name;

    public:
        assign_to(std::string value) : m_variable_name(value) {}
        assign_to(std::string_view value) : m_variable_name(value.begin(), value.end()) {}
        virtual void execute(sqf::runtime::runtime& vm) const override
        {
            auto& context = vm.context_active();

            auto value = vm.context_active().pop_value();
            // Real Arma's assignment always evaluates to nil - without
            // pushing one, an assignment as a code block's last statement
            // (very common: "private _x = ...; _y = ...;") leaves nothing
            // for the block's caller to consume. A single-statement block
            // can appear to work by accident (picking up an unrelated,
            // pre-existing stack value) while a multi-statement block whose
            // prior statement boundary clears the stack cannot - see
            // end_statement, which unconditionally clears between
            // statements. Push nil unconditionally, including on the error
            // paths below, so this assignment's own result is always well
            // defined regardless of what its right-hand side did.
            sqf::runtime::value nilResult;
            if (!value.has_value())
            {
                if (context.weak_error_handling())
                {
                    vm.__logmsg(logmessage::runtime::FoundNoValueWeak(diag_info()));
                }
                else
                {
                    vm.__logmsg(logmessage::runtime::FoundNoValue(diag_info()));
                }
                vm.context_active().push_value(nilResult);
                return;
            }
            else if (value->is<sqf::types::t_nothing>())
            {
                vm.__logmsg(logmessage::runtime::AssigningNilValue(diag_info(), m_variable_name));
            }
            if (m_variable_name.empty()) { vm.context_active().push_value(nilResult); return; }
            if (m_variable_name[0] == '_')
            {
                for (auto it = context.frames_rbegin(); it != context.frames_rend(); ++it)
                {
                    if (it->contains(m_variable_name))
                    {
                        (*it)[m_variable_name] = *value;
                        vm.context_active().push_value(nilResult);
                        return;
                    }
                }
                context.current_frame()[m_variable_name] = *value;
            }
            else
            {
                context.current_frame().globals_value_scope()->at(m_variable_name) = *value;
            }
            vm.context_active().push_value(nilResult);
        }
        virtual std::string to_string() const override { return std::string("ASSIGNTO ") + m_variable_name; }
        std::string_view variable_name() const { return m_variable_name; }

        virtual std::optional<std::string> reconstruct(
            std::vector<sqf::runtime::instruction::sptr>::const_reverse_iterator& current,
            std::vector<sqf::runtime::instruction::sptr>::const_reverse_iterator end,
            short parent_precedence, bool left_from_binary) const override
        {
            if (++current == end)
            {
                return {};
            }
            auto exp = (*current)->reconstruct(current, end, 10, false);
            if (!exp.has_value())
            {
                return {};
            }
            return m_variable_name + " = " + *exp;
        }

        virtual bool equals(const instruction* p_other) const override
        {
            auto casted = dynamic_cast<const assign_to*>(p_other);
            return casted != nullptr && casted->m_variable_name == m_variable_name;
        }
    };
}