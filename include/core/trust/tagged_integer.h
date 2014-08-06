/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef CORE_TRUST_TAGGED_INTEGER_H_
#define CORE_TRUST_TAGGED_INTEGER_H_

#include <cstdint>

#include <ostream>
#include <type_traits>

#include <sys/types.h>

namespace core
{
namespace trust
{
/** @brief Helper structure for tagging integer types with certain semantics. */
template<typename Tag, typename Integer>
struct TaggedInteger
{
    /** @brief We bail out if the Integral type is not an integral one. */
    static_assert(std::is_integral<Integer>::value, "Integer has to be an integral type");

    /** @brief Stores the Tag type. */
    typedef Tag TagType;
    /** @brief Stores the Integer type. */
    typedef Integer IntegerType;

    /** @brief Construct an instance with a default value. */
    TaggedInteger() : value{}
    {
    }

    /** @brief Construct an instance from an existing integer type. */
    explicit TaggedInteger(Integer value) : value{value}
    {
    }

    /** @brief The contained integer value. */
    Integer value;
};

/** @brief Returns true iff both tagged integer instances are equal. */
template<typename Tag, typename Integer>
inline bool operator==(const TaggedInteger<Tag, Integer>& lhs, const TaggedInteger<Tag, Integer>& rhs)
{
    return lhs.value == rhs.value;
}

/** @brief Returns true iff both tagged integer instances are not equal. */
template<typename Tag, typename Integer>
inline bool operator!=(const TaggedInteger<Tag, Integer>& lhs, const TaggedInteger<Tag, Integer>& rhs)
{
    return lhs.value != rhs.value;
}

/** @brief Returns true iff the left-hand-side integer instance is smaller than the right-hand-side. */
template<typename Tag, typename Integer>
inline bool operator<(const TaggedInteger<Tag, Integer>& lhs, const TaggedInteger<Tag, Integer>& rhs)
{
    return lhs.value < rhs.value;
}

/** @brief Pretty prints a tagged integer. */
template<typename Tag, typename Integer>
inline std::ostream& operator<<(std::ostream& out, const TaggedInteger<Tag, Integer>& ti)
{
    return out << ti.value;
}

namespace tag
{
// Tags a group id
struct Gid {};
// Tags a process id
struct Pid {};
// Tags a user id
struct Uid {};
// Tags a service-specific feature
struct Feature {};
}

/** @brief Our internal group id type. */
typedef TaggedInteger<tag::Gid, gid_t> Gid;
/** @brief Our internal process id type. */
typedef TaggedInteger<tag::Pid, pid_t> Pid;
/** @brief Our internal user id type. */
typedef TaggedInteger<tag::Uid, uid_t> Uid;
/** @brief Our internal service-feature type. */
typedef TaggedInteger<tag::Feature, std::uint64_t> Feature;
}
}

#endif // CORE_TRUST_TAGGED_INTEGER_H_
