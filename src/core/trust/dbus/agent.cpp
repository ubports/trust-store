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

#include <core/trust/dbus/agent.h>

boost::format core::trust::dbus::Agent::default_service_name_pattern()
{
    return boost::format("core.trust.dbus.Agent.%1%");
}

core::dbus::types::ObjectPath core::trust::dbus::Agent::default_object_path()
{
    return core::dbus::types::ObjectPath{"/core/trust/dbus/Agent"};
}

const std::string& core::trust::dbus::Agent::name()
{
    static const std::string s{"core.trust.dbus.Agent"};
    return s;
}

core::trust::dbus::Agent::Errors::CouldNotDetermineConclusiveAnswer::CouldNotDetermineConclusiveAnswer()
    : std::runtime_error("Could not determine conclusive answer to trust request.")
{
}

core::trust::dbus::Agent::Stub::Stub(const core::dbus::Object::Ptr& object)
    : object{object}
{
}

core::trust::Request::Answer core::trust::dbus::Agent::Stub::authenticate_request_with_parameters(const core::trust::Agent::RequestParameters& parameters)
{
    auto result = object->transact_method
            <
                Methods::AuthenticateRequestWithParameters,
                core::trust::Request::Answer
            >(parameters);

    if (result.is_error()) throw std::runtime_error
    {
        result.error().print()
    };

    return result.value();
}

core::trust::dbus::Agent::Skeleton::Skeleton(const Configuration& config)
    : configuration(config)
{
    configuration.object->install_method_handler<Methods::AuthenticateRequestWithParameters>([this](const core::dbus::Message::Ptr& in)
    {
        core::trust::Agent::RequestParameters params; in->reader() >> params;

        core::dbus::Message::Ptr reply;
        try
        {
            reply = core::dbus::Message::make_method_return(in);
            reply->writer() << authenticate_request_with_parameters(params);
        } catch(const std::exception& e)
        {
            // TODO(tvoss): we should report the error locally here.
            reply = core::dbus::Message::make_error(in, Errors::CouldNotDetermineConclusiveAnswer::name, "");
        } catch(...)
        {
            reply = core::dbus::Message::make_error(in, Errors::CouldNotDetermineConclusiveAnswer::name, "");
        }

        configuration.bus->send(reply);
    });
}

core::trust::Request::Answer core::trust::dbus::Agent::Skeleton::authenticate_request_with_parameters(const core::trust::Agent::RequestParameters& request)
{
    return configuration.agent(request);
}


/**
 * @brief create_per_user_agent_for_bus_connection creates a trust::Agent implementation communicating with a remote agent
 * implementation living in the same user session.
 * @param connection An existing DBus connection.
 * @param service The name of the service we are operating for.
 * @throws std::runtime_error in case of issues.
 */
std::shared_ptr<core::trust::Agent> core::trust::dbus::create_per_user_agent_for_bus_connection(
        const std::shared_ptr<core::dbus::Bus>& connection,
        const std::string& service_name)
{
    auto object =
            core::dbus::Service::use_service(connection, (core::trust::dbus::Agent::default_service_name_pattern() % service_name).str())
                ->object_for_path(core::trust::dbus::Agent::default_object_path());

    return std::shared_ptr<core::trust::dbus::Agent::Stub>
    {
        new core::trust::dbus::Agent::Stub{object}
    };
}
