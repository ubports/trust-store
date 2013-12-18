/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef CORE_TRUST_REQUEST_H_
#define CORE_TRUST_REQUEST_H_

#include <core/trust/visibility.h>

#include <cstdint>

#include <chrono>
#include <ostream>
#include <string>

namespace core
{
namespace trust
{
struct CORE_TRUST_DLL_PUBLIC Request
{
    /** @brief Requests are timestamped with wallclock time. */
    typedef std::chrono::system_clock::time_point Timestamp;

    /** @brief Default feature identifier. */
    static const unsigned int default_feature = 0;

    /** @brief Enumerates the possible answers given by a user. */
    enum class Answer
    {
        denied, ///< Nope, I do not trust this application.
        granted ///< Yup, I do trust this application.
    };

    inline bool operator==(const Request& rhs) const
    {
        return from == rhs.from &&
               feature == rhs.feature &&
               when == rhs.when &&
               answer == rhs.answer;
    }

    inline friend std::ostream& operator<<(std::ostream& out, const Request::Answer& a)
    {
        switch (a)
        {
        case Request::Answer::granted: out << "granted"; break;
        case Request::Answer::denied: out << "denied"; break;
        }

        return out;
    }

    inline friend std::ostream& operator<<(std::ostream& out, const Request& r)
    {
        out << "Request("
            << "from: " << r.from << ", "
            << "feature: " << r.feature << ", "
            << "when: " << r.when.time_since_epoch().count() << ", "
            << "answer: " << r.answer << ")";

        return out;
    }

    /** The application id of the application that resulted in the request. */
    std::string from;
    /** An application-specific feature identifier. */
    std::uint64_t feature;
    /** When the request happened in wallclock time. */
    Timestamp when;
    /** The user's answer. */
    Answer answer;
};
}
}

#endif // CORE_TRUST_REQUEST_H_
