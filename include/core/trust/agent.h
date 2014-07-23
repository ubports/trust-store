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

#ifndef CORE_TRUST_AGENT_H_
#define CORE_TRUST_AGENT_H_

#include <core/trust/request.h>
#include <core/trust/tagged_integer.h>
#include <core/trust/visibility.h>

#include <cstdint>

namespace core
{
namespace trust
{
/** @brief Abstracts user-prompting functionality. */
class CORE_TRUST_DLL_PUBLIC Agent
{
public:
    /** @cond */
    Agent() = default;
    virtual ~Agent() = default;

    Agent(const Agent&) = delete;
    Agent(Agent&&) = delete;
    Agent& operator=(const Agent&) = delete;
    Agent& operator=(Agent&&) = delete;
    /** @endcond */

    /** @brief Summarizes all parameters for processing a trust request. */
    struct RequestParameters
    {
        /** @brief The user id under which the requesting application runs. */
        core::trust::Uid application_uid;
        /** @brief The process id of the requesting application. */
        core::trust::Pid application_pid;
        /** @brief The id of the requesting application. */
        std::string application_id;
        /** @brief The service-specific feature identifier. */
        Feature feature;
        /** @brief An extended description that should be presented to the user on prompting. */
        std::string description;
    };

    /**
     * @brief Authenticates the given request and returns the user's answer.
     * @param parameters [in] Describe the request.
     */
    virtual Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters) = 0;
};

/** @brief Returns true iff lhs and rhs are equal. */
CORE_TRUST_DLL_PUBLIC bool operator==(const Agent::RequestParameters& lhs, const Agent::RequestParameters& rhs);
}
}

#endif // CORE_TRUST_AGENT_H_
