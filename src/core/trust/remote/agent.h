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

#ifndef CORE_TRUST_REMOTE_AGENT_H_
#define CORE_TRUST_REMOTE_AGENT_H_

#include <core/trust/agent.h>
#include <core/trust/request.h>

#include <core/trust/visibility.h>

#include <type_traits>

namespace core
{
namespace trust
{
namespace remote
{
// Abstracts listeners for incoming requests, possible implementations:
//   * Listening to a socket
//   * DBus
struct Agent
{
    // Models the sending end of a remote agent, meant to be used by trusted helpers.
    struct CORE_TRUST_DLL_PUBLIC Stub  : public core::trust::Agent
    {
        Stub() = default;
        virtual ~Stub() = default;

        // From core::trust::Agent
        virtual Request::Answer authenticate_request_with_parameters(const RequestParameters& request);

        // Sends out the request to the receiving end, either returning an answer
        // or throwing an exception if no conclusive answer could be obtained from
        // the user.
        virtual core::trust::Request::Answer send(const RequestParameters& parameters) = 0;
    };

    // Models the receiving end of a remote agent, meant to be used by the trust store daemon.
    struct CORE_TRUST_DLL_PUBLIC  Skeleton : public core::trust::Agent
    {
        // Constructs a new Skeleton instance, installing impl for handling actual requests.
        Skeleton(const std::shared_ptr<Agent>& impl);

        virtual ~Skeleton() = default;

        // From core::trust::Agent, dispatches to the actual implementation.
        virtual core::trust::Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters);

        // The actual agent implementation that we are dispatching to.
        std::shared_ptr<core::trust::Agent> impl;
    };
};
}
}
}

#endif // CORE_TRUST_REMOTE_AGENT_H_
