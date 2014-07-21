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
#include <core/trust/visibility.h>

namespace core
{
namespace trust
{
// Forward-declarations.
struct RequestParameters;

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

    /**
     * @brief Presents the given request to the user, returning the user-provided answer.
     * @param app_uid The user under which the requesting app is running.
     * @param app_pid The process id of the requesting application.
     * @param app_id The application id of the requesting application.
     * @param description Extended description of the trust request.
     */
    virtual Request::Answer prompt_user_for_request(uid_t app_uid, pid_t app_pid, const std::string& app_id, const std::string& description) = 0;
};
}
}

#endif // CORE_TRUST_AGENT_H_
