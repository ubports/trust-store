/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef CORE_TRUST_PRIVILEGE_ESCALATION_PREVENTION_AGENT_H_
#define CORE_TRUST_PRIVILEGE_ESCALATION_PREVENTION_AGENT_H_

#include <core/trust/agent.h>

#include <stdexcept>
#include <functional>

namespace core
{
namespace trust
{
// A PrivilegeEscalationPreventionAgent ensures that requests originating from an application
// running under a different user than the current one are rejected immediately, thereby preventing
// from privilege escalation issues.
class CORE_TRUST_DLL_PUBLIC PrivilegeEscalationPreventionAgent : public core::trust::Agent
{
public:
    // An Error is thrown if a potential privilege escalation attack has been detected.
    struct Error : public std::runtime_error
    {
        // Error creates an instance, providing details about the escalation issue.
        Error();
    };

    // A UserIdFunctor queries the user id under which the current process runs.
    typedef std::function<Uid()> UserIdFunctor;

    // default_user_id_functor returns a UserIdFunctor querying the current user id from the system.
    static UserIdFunctor default_user_id_functor();

    // PrivilegeEscalationPreventionAgent creates a new instance that queries the current user,
    // forwarding valid requests to impl.
    PrivilegeEscalationPreventionAgent(const UserIdFunctor& uid_functor, const std::shared_ptr<Agent>& impl);

    // From core::trust::Agent
    Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters) override;

private:
    UserIdFunctor uid_functor;
    std::shared_ptr<Agent> impl;
};
}
}

#endif // CORE_TRUST_PRIVILEGE_ESCALATION_PREVENTION_AGENT_H_
