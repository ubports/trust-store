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
/**
 * @brief The Request struct encapsulates information about a trust request answered by the user.
 *
 * A Request is the main entity managed by the trust-store API. Whenever an
 * application tries to access the functionality offered by a trusted helper,
 * the trusted helper checks whether the application has issued a request
 * before. If a query against the trust store returns yes and the user
 * previously granted trust to the application, the application's request to
 * the trusted helpers functionality is granted. If the user previously
 * rejected the request, the app's request is denied. If no previous request
 * can be found, the trusted helper issues a question to the user, collects the
 * answer and transacts the complete request to the store.
 *
 */
struct CORE_TRUST_DLL_PUBLIC Request
{
    /** @brief Duration in wallclock time. */
    typedef std::chrono::system_clock::duration Duration;
    /** @brief Requests are timestamped with wallclock time. */
    typedef std::chrono::system_clock::time_point Timestamp;

    /** @brief Default feature identifier. */
    static constexpr const unsigned int default_feature = 0;

    /** @brief Enumerates the possible answers given by a user. */
    enum class Answer
    {
        denied, ///< Nope, I do not trust this application.
        granted ///< Yup, I do trust this application.
    };

    /** The application id of the application that resulted in the request. */
    std::string from;
    /** An application-specific feature identifier. */
    std::uint64_t feature;
    /** When the request happened in wallclock time. */
    Timestamp when;
    /** The user's answer. */
    Answer answer;
};
/**
 * @brief operator == compares two Requests for equality.
 * @param lhs [in] The left-hand-side of the comparison.
 * @param rhs [in] The right-hand-side of the comparison.
 * @return true iff both requests are equal.
 */
CORE_TRUST_DLL_PUBLIC bool operator==(const Request& lhs, const Request& rhs);

/**
 * @brief operator << pretty prints answers to the provided output stream.
 * @param out [in, out] The stream to print to.
 * @param a The answer to be printed.
 * @return The output stream.
 */
CORE_TRUST_DLL_PUBLIC std::ostream& operator<<(std::ostream& out, const Request::Answer& a);

/**
 * @brief operator << pretty prints a request to the provided output stream.
 * @param out [in, out] The stream to print to.
 * @param r The request to be printed.
 * @return The output stream.
 */
CORE_TRUST_DLL_PUBLIC std::ostream& operator<<(std::ostream& out, const Request& r);
}
}

#endif // CORE_TRUST_REQUEST_H_
