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
#include <memory>
#include <ostream>
#include <string>

namespace core
{
namespace trust
{
// Forward declarations
class Agent;
class Store;

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
        granted, ///< Yup, I do trust this application.
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

/** @brief Summarizes all parameters for processing a trust request. */
struct CORE_TRUST_DLL_PUBLIC RequestParameters
{
    /** @brief The Agent implementation to dispatch a request to the user. */
    std::shared_ptr<Agent> agent;
    /** @brief The trust store to be used for caching purposes. */
    std::shared_ptr<Store> store;
    /** @brief The process id of the requesting application. */
    pid_t application_pid;
    /** @brief The id of the requesting application. */
    std::string application_id;
    /** @brief The service-specific feature identifier. */
    std::uint64_t feature;
    /** @brief An extended description that should be presented to the user on prompting. */
    std::string description;
};

/**
 * @brief Processes an incoming trust-request by an application, tries to lookup a previous reply before
 * issuing a prompt request via the given agent to the user. On return, the given trust-store is up-to-date.
 *
 * @throws std::exception To indicate that no conclusive answer could be resolved from either the store or
 * the user. In that case, the state of the store instance passed in to the function is not altered.
 *
 * The following code snippet illustrates how to use the function:
 *
 * @code
 * struct Service
 * {
 *     static constexpr std::uint64_t default_feature = 0;
 *
 *     void on_session_requested(pid_t app_pid, const std::string& app_id)
 *     {
 *         core::trust::RequestParameters params
 *         {
 *             trust.agent,
 *             trust.store,
 *             app_pid,
 *             app_id,
 *             default_feature,
 *             "Application " + app_id + " wants to access the example service."
 *         };
 *
 *         switch(process_trust_request(params))
 *         {
 *             case core::trust::Request::Answer::granted:
 *                 // Create session and get back to application with session credentials.
 *                 break;
 *             case core::trust::Request::Answer::denied:
 *                 // Deny session creation and inform application.
 *                 break;
 *         }
 *     }
 *
 *     struct
 *     {
 *         // We use Mir's trust session support to request the prompting UI.
 *         std::shared_ptr<core::trust::Agent> agent
 *         {
 *             core::trust::mir::make_agent_for_existing_connection(mir_connection)
 *         };
 *
 *         std::shared_ptr<core::trust::Store> store
 *         {
 *             core::trust::create_default_store("my.example.service");
 *         };
 *     } trust;
 * };
 * @endcode
 */
CORE_TRUST_DLL_PUBLIC Request::Answer process_trust_request(const RequestParameters& params);
}
}

#endif // CORE_TRUST_REQUEST_H_
