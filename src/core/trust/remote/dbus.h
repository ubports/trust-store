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

#ifndef CORE_TRUST_REMOTE_DBUS_H_
#define CORE_TRUST_REMOTE_DBUS_H_

#include <core/trust/remote/agent.h>
#include <core/trust/remote/helpers.h>

#include <core/trust/dbus/agent.h>
#include <core/trust/dbus/agent_registry.h>

#include <unistd.h>
#include <sys/types.h>

namespace core
{
namespace trust
{
namespace remote
{
namespace dbus
{
constexpr const char* default_service_name_prefix
{
    "core.trust.dbus.Agent"
};

constexpr const char* default_agent_registry_path
{
    "/core/trust/dbus/AgentRegistry"
};

// Abstracts listeners for incoming requests, possible implementations:
//   * Listening to a socket
//   * DBus
struct CORE_TRUST_DLL_PUBLIC Agent
{
    // Models the sending end of a remote agent, meant to be used by trusted helpers.
    struct Stub : public core::trust::remote::Agent::Stub
    {
        // All creation time parameters go here.
        struct Configuration
        {
            // Object to install an implementation of
            // core.trust.dbus.AgentRegistry on.
            core::dbus::Object::Ptr object;
            // Bus-connection for sending out replies.
            core::dbus::Bus::Ptr bus;
        };

        // Sets up the stub.
        Stub(const Configuration& configuration);

        // Delivers the request described by the given parameters to the other side.
        core::trust::Request::Answer send(const RequestParameters& parameters) override;

        // Our actual agent registry implementation.
        core::trust::LockingAgentRegistry agent_registry;
        // That we expose over the bus.
        core::trust::dbus::AgentRegistry::Skeleton agent_registry_skeleton;
    };

    // Models the receiving end of a remote agent, meant to be used by the trust store daemon.
    struct Skeleton : public core::trust::remote::Agent::Skeleton
    {
        // All creation time parameters go here.
        struct Configuration
        {
            // The actual local agent implementation.
            std::shared_ptr<Agent> impl;
            // The remote object implementing core.trust.dbus.AgentRegistry.
            core::dbus::Object::Ptr agent_registry_object;
            // The service that objects implementing core.trust.dbus.Agent should be added to.
            core::dbus::Service::Ptr service;
            // The underlying bus instance.
            core::dbus::Bus::Ptr bus;
            // A helper for querying the application id for a given uid.
            helpers::AppIdResolver resolve_app_id;
        };

        // Constructs a new Skeleton instance, installing impl for handling actual requests.
        Skeleton(const Configuration& configuration);

        // From core::trust::Agent, dispatches to the actual implementation.
        core::trust::Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters);

        // Stub for accessing the remote agent registry.
        core::trust::dbus::AgentRegistry::Stub agent_registry_stub;
    };
};
}
}
}
}

#endif // CORE_TRUST_REMOTE_DBUS_H_
