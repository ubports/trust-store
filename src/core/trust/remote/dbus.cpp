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

#include <core/trust/remote/dbus.h>

#include <core/trust/dbus_agent.h>

core::trust::remote::dbus::Agent::Stub::Stub(const core::trust::remote::dbus::Agent::Stub::Configuration& configuration)
    : agent_registry_skeleton
      {
          core::trust::dbus::AgentRegistry::Skeleton::Configuration
          {
              configuration.object,
              configuration.bus,
              agent_registry
          }
      }
{
}

core::trust::Request::Answer core::trust::remote::dbus::Agent::Stub::send(const core::trust::Agent::RequestParameters& parameters)
{
    if (not agent_registry.has_agent_for_user(parameters.application.uid))
        return core::trust::Request::Answer::denied;

    auto agent = agent_registry.agent_for_user(parameters.application.uid);

    return agent->authenticate_request_with_parameters(parameters);
}

core::trust::remote::dbus::Agent::Skeleton::Skeleton(const core::trust::remote::dbus::Agent::Skeleton::Configuration& configuration)
    : core::trust::remote::Agent::Skeleton{configuration.impl},
      agent_registry_stub
      {
          core::trust::dbus::AgentRegistry::Stub::Configuration
          {
              configuration.agent_registry_object,
              core::trust::dbus::AgentRegistry::Stub::counting_object_path_generator(),
              configuration.service,
              configuration.bus
          }
      }
{
    agent_registry_stub.register_agent_for_user(core::trust::Uid{::getuid()}, configuration.impl);
}

core::trust::Request::Answer core::trust::remote::dbus::Agent::Skeleton::authenticate_request_with_parameters(const core::trust::Agent::RequestParameters& parameters)
{
    return remote::Agent::Skeleton::authenticate_request_with_parameters(parameters);
}

std::shared_ptr<core::trust::Agent> core::trust::dbus::create_multi_user_agent_for_bus_connection(
        const std::shared_ptr<core::dbus::Bus>& connection,
        const std::string& service_name)
{
    auto dbus_service_name = core::trust::remote::dbus::default_service_name_prefix + std::string{"."} + service_name;

    auto service = core::dbus::Service::add_service(connection, dbus_service_name);
    auto object = service->add_object_for_path(core::dbus::types::ObjectPath
    {
        core::trust::remote::dbus::default_agent_registry_path
    });

    core::trust::remote::dbus::Agent::Stub::Configuration config
    {
        object,
        connection
    };

    return std::shared_ptr<core::trust::remote::dbus::Agent::Stub>
    {
        new core::trust::remote::dbus::Agent::Stub
        {
            config
        }
    };
}
