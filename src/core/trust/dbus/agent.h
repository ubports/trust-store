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

#ifndef CORE_TRUST_DBUS_AGENT_H_
#define CORE_TRUST_DBUS_AGENT_H_

#include <core/trust/agent.h>

#include <core/trust/dbus/codec.h>
#include <core/trust/remote/helpers.h>

#include <core/dbus/macros.h>
#include <core/dbus/object.h>

namespace core
{
namespace trust
{
namespace dbus
{
struct Agent
{
    static const std::string& name()
    {
        static const std::string s
        {
            "core.trust.dbus.Agent"
        };
        return s;
    }

    struct Errors
    {
        struct CouldNotDetermineConclusiveAnswer : public std::runtime_error
        {
            static constexpr const char* name
            {
                "core.trust.dbus.Agent.Errors.CouldNotDetermineConclusiveAnswer"
            };

            CouldNotDetermineConclusiveAnswer()
                : std::runtime_error("Could not determine conclusive answer to trust request.")
            {
            }
        };
    };

    // Just for scoping purposes
    struct Methods
    {
        // And thus not constructible
        Methods() = delete;

        DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(AuthenticateRequestWithParameters, Agent, 10000)
    };

    class Stub : public core::trust::Agent
    {
    public:
        Stub(const core::dbus::Object::Ptr& object)
            : object{object}
        {
        }

        Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters) override
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

    private:
        core::dbus::Object::Ptr object;
    };

    class Skeleton : public core::trust::Agent
    {
    public:
        // All creation time parameters go here.
        struct Configuration
        {
            // The object that should implement core.trust.Agent
            core::dbus::Object::Ptr object;
            // The underlying bus instance
            core::dbus::Bus::Ptr bus;
            // The agent implementation
            std::function<core::trust::Request::Answer(const core::trust::Agent::RequestParameters&)> agent;
        };

        Skeleton(const Configuration& config)
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

        Request::Answer authenticate_request_with_parameters(const RequestParameters& request) override
        {
            return configuration.agent(request);
        }

    private:
        // We just store all creation time parameters.
        Configuration configuration;
    };
};
}
}
}

#endif // CORE_TRUST_DBUS_AGENT_H_
