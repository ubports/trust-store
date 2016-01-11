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

#include <core/trust/privilege_escalation_prevention_agent.h>

#include <unistd.h>
#include <sys/types.h>

core::trust::PrivilegeEscalationPreventionAgent::Error::Error() : std::runtime_error{"Potential privilege escalation attack detected."}
{
}

core::trust::PrivilegeEscalationPreventionAgent::UserIdFunctor core::trust::PrivilegeEscalationPreventionAgent::default_user_id_functor()
{
    return []()
    {
        return core::trust::Uid{::getuid()};
    };
}

core::trust::PrivilegeEscalationPreventionAgent::PrivilegeEscalationPreventionAgent(
        const UserIdFunctor& uid_functor,
        const std::shared_ptr<core::trust::Agent>& impl)
    : uid_functor{uid_functor},
      impl{impl}
{
    if (not impl) throw std::runtime_error
    {
        "Missing agent implementation."
    };
}

core::trust::Request::Answer core::trust::PrivilegeEscalationPreventionAgent::authenticate_request_with_parameters(const core::trust::Agent::RequestParameters& parameters)
{
    if (uid_functor() != parameters.application.uid) throw Error{};
    return impl->authenticate_request_with_parameters(parameters);
}
