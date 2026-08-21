#pragma once
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <algorithm>

#include "type.h"

namespace sqf::runtime
{
    class runtime;
    class data
    {
        template <typename T>
        struct empty_delete
        {
            empty_delete() /* noexcept */
            {
            }

            template <typename U>
            empty_delete(const empty_delete<U>&,
                typename std::enable_if<
                std::is_convertible<U*, T*>::value
                >::type* = nullptr) /* noexcept */
            {
            }

            void operator()(T* const) const /* noexcept */
            {
                // do nothing
            }
        };
    protected:
        /// <summary>
        /// Performs the actual comparison.
        /// Compares this value against the other value and returns
        /// true if they are equivalent.
        /// If invariant false, comparison should be case-sensitive.
        /// </summary>
        /// <param name="other">The other value</param>
        /// <param name="invariant">Wether operation should be case sensitive (false) or not (true)</param>
        /// <returns>The result of the comparison</returns>
        virtual bool do_equals(std::shared_ptr<data> other, bool invariant) const = 0;

    public:

        /// <summary>
        /// Returns the SQF representation of this datatype.
        /// SQF-representation here means that this could get printed 
        /// out to file directly and parsed again.
        /// </summary>
        /// <param name="runtime">
        /// Valid instance of runtime the current runtime,
        /// requesting the SQF representation.
        /// </param>
        /// <returns>SQF-Representation of this data.</returns>
        virtual std::string to_string_sqf() const = 0;

        /// <summary>
        /// Returns a string representation of this datatype.
        /// </summary>
        /// <returns>String representing this datatype.</returns>
        virtual std::string to_string() const = 0;

        /// <summary>
        /// Compares this value against the other value and returns
        /// true if they are equivalent.
        /// If invariant false, comparison should be case-sensitive.
        /// </summary>
        /// <param name="other">The other value</param>
        /// <param name="invariant">Wether operation should be case sensitive (false) or not (true)</param>
        /// <returns>The result of the comparison</returns>
        bool equals(std::shared_ptr<data> other, bool invariant = false) const
        {
            if (other->type() != type()) { return false; }
            if (other.get() == this) { return true; }
            // Containers (ARRAY, HASHMAP) recurse into their elements'
            // equals() here. Two distinct but structurally circular graphs
            // (e.g. two different objects that both hold self/cross
            // references back into themselves) can keep re-entering the
            // comparison of this exact (this, other) pair forever, since
            // neither side is ever pointer-identical to trip the check
            // above - that recurses until the stack overflows. Track pairs
            // currently being compared on this thread's call stack; seeing
            // the same pair again means we're in a cycle, so stop and
            // report unequal instead of recursing further.
            thread_local std::vector<std::pair<const data*, const data*>> s_equals_in_progress;
            std::pair<const data*, const data*> key(this, other.get());
            if (std::find(s_equals_in_progress.begin(), s_equals_in_progress.end(), key) != s_equals_in_progress.end())
            {
                return false;
            }
            s_equals_in_progress.push_back(key);
            bool result = do_equals(other, invariant);
            s_equals_in_progress.pop_back();
            return result;
        }

        /// <summary>
        /// Actual type of this.
        /// </summary>
        /// <returns></returns>
        virtual ::sqf::runtime::type type() const = 0;

        virtual std::size_t hash() const = 0;
    };
}
namespace std
{
    template<> struct hash<sqf::runtime::data>
    {
        std::size_t operator()(sqf::runtime::data const& s) const noexcept
        {
            return s.hash();
        }
    };
}