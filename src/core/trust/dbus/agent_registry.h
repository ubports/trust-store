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

#ifndef CORE_TRUST_DBUS_AGENT_REGISTRY_H_
#define CORE_TRUST_DBUS_AGENT_REGISTRY_H_

#include <core/trust/dbus/agent.h>
#include <core/trust/dbus/codec.h>

#include <core/dbus/macros.h>
#include <core/dbus/object.h>

namespace core
{
namespace trust
{
// A thread-safe AgentRegistry implementation.
class LockingAgentRegistry : public Agent::Registry
{
public:
    // From AgentRegistry
    void register_agent_for_user(const core::trust::Uid& uid, const std::shared_ptr<core::trust::Agent>& agent) override
    {
        std::lock_guard<std::mutex> lg(guard);
        registered_agents[uid] = agent;
    }

    void unregister_agent_for_user(const core::trust::Uid& uid) override
    {
        std::lock_guard<std::mutex> lg(guard);
        registered_agents.erase(uid);
    }

    // And some convenience
    // Returns true iff the registry knows about an agent for the given uid.
    bool has_agent_for_user(const core::trust::Uid& uid) const
    {
        std::lock_guard<std::mutex> lg(guard);
        return registered_agents.count(uid) > 0;
    }

    // Returns the agent implementation for the given uid
    // or throws std::out_of_range if no agent is known for the uid.
    std::shared_ptr<Agent> agent_for_user(const core::trust::Uid& uid) const
    {
        std::lock_guard<std::mutex> lg(guard);
        return registered_agents.at(uid);
    }

private:
    mutable std::mutex guard;
    std::map<core::trust::Uid, std::shared_ptr<core::trust::Agent>> registered_agents;
};

namespace dbus
{
// Encapsulates stubs and skeletons for exposing/resolving an agent registry
// over the bus.
struct AgentRegistry
{
    // To safe some typing
    typedef std::shared_ptr<AgentRegistry> Ptr;

    // The interface name
    static const std::string& name()
    {
        static const std::string s
        {
            "core.trust.dbus.AgentRegistry"
        };
        return s;
    }

    // Encapsulates errors that can occur when
    // using the stub or the skeleton.
    struct Errors
    {
        // The remote implementation could not register the
        // agent for the given user.
        struct CouldNotRegisterAgentForUser
        {
            static constexpr const char* name
            {
                "core.trust.dbus.AgentRegistry.CouldNotRegisterAgentForUser"
            };
        };
    };

    // Encapsulates all methods known to the remote end.
    struct Methods
    {
        // Not constructible.
        Methods() = delete;

        // Maps to register_agent_for_user
        DBUS_CPP_METHOD_DEF(RegisterAgentForUser, AgentRegistry)
        // Maps to unregister_agent_for_user
        DBUS_CPP_METHOD_DEF(UnregisterAgentForUser, AgentRegistry)
    };

    // A DBus stub implementation of core::trust::AgentRegistry.
    struct Stub : public core::trust::Agent::Registry
    {
        // Functor for generating unique object paths.
        typedef std::function<core::dbus::types::ObjectPath(const core::trust::Uid&)> ObjectPathGenerator;

        static ObjectPathGenerator counting_object_path_generator()
        {
            return [](const core::trust::Uid&)
            {
                static int counter{0};
                return core::dbus::types::ObjectPath{"/core/trust/dbus/Agent/" + std::to_string(counter++)};
            };
        }

        // All creation time parameters go here.
        struct Configuration
        {
            // The remote object implementing core.trust.dbus.AgentRegistry.
            core::dbus::Object::Ptr object;
            // The object generator instance.
            ObjectPathGenerator object_path_generator;
            // The local service instance to add objects to.
            core::dbus::Service::Ptr service;
            // The underlying bus instance.
            core::dbus::Bus::Ptr bus;
        };

        // Initializes stub access to the Stub.
        Stub(const Configuration& configuration)
            : configuration(configuration)
        {
        }

        // Calls into the remote implementation to register the given agent implementation.
        // Throws std::runtime_error and std::logic_error in case of issues.
        void register_agent_for_user(const core::trust::Uid& uid, const std::shared_ptr<core::trust::Agent>& impl) override
        {
            // We sample a path for the given uid
            auto path = configuration.object_path_generator(uid);
            // And construct the skeleton instance.
            auto skeleton = std::shared_ptr<core::trust::dbus::Agent::Skeleton>
            {
                new core::trust::dbus::Agent::Skeleton
                {
                    core::trust::dbus::Agent::Skeleton::Configuration
                    {
                        configuration.service->add_object_for_path(path),
                        configuration.bus,
                        std::bind(&core::trust::Agent::authenticate_request_with_parameters, impl, std::placeholders::_1)
                    }
                }
            };

            // And announce the agent.
            auto result = configuration.object->transact_method<Methods::RegisterAgentForUser, void>(uid, path);

            if (result.is_error()) throw std::runtime_error
            {
                result.error().print()
            };

            // All good, finally updating our local registry.
            locking_agent_registry.register_agent_for_user(uid, skeleton);
        }

        // Calls into the remote implementation to unregister any agent registered for the given uid.
        // Throws std::runtime_error and std::logic_error in case of issues.
        void unregister_agent_for_user(const core::trust::Uid& uid) override
        {
            auto result = configuration.object->transact_method<Methods::UnregisterAgentForUser, void>(uid);

            // Updating our local registry even if the call to the remote end
            // might have failed.
            locking_agent_registry.unregister_agent_for_user(uid);

            if (result.is_error()) throw std::runtime_error
            {
                result.error().print()
            };
        }

        // We just store all creation-time arguments.
        Configuration configuration;
        // Our local registry of agents
        LockingAgentRegistry locking_agent_registry;
    };

    // A DBus skeleton implementation of core::trust::AgentRegistry.
    struct Skeleton : public core::trust::Agent::Registry
    {
        // Creation time parameters go here.
        struct Configuration
        {
            // Object to install an implementation of
            // core.trust.dbus.AgentRegistry on.
            core::dbus::Object::Ptr object;
            // Bus-connection for sending out replies.
            core::dbus::Bus::Ptr bus;
            // The actual implementation.
            core::trust::Agent::Registry& impl;
        };

        // Setups up handlers for:
        //   Methods::RegisterAgentForUser
        //   Methods::UnregisterAgentForUser
        Skeleton(const Configuration& config)
            : configuration(config)
        {
            configuration.object->install_method_handler<Methods::RegisterAgentForUser>([this](const core::dbus::Message::Ptr& in)
            {
                core::trust::Uid uid; core::dbus::types::ObjectPath path;
                in->reader() >> uid >> path;

                // Assemble the stub Agent from the given parameters.
                auto service = core::dbus::Service::use_service(configuration.bus, in->sender());
                auto object = service->object_for_path(path);

                std::shared_ptr<core::trust::dbus::Agent::Stub> agent
                {
                    new core::trust::dbus::Agent::Stub
                    {
                        object
                    }
                };

                core::dbus::Message::Ptr reply;

                // Forward the registration request to the actual implementation.
                try
                {
                    configuration.impl.register_agent_for_user(uid, agent);
                    reply = core::dbus::Message::make_method_return(in);
                } catch(const std::exception&)
                {
                    // TODO(tvoss): We should report the exception here.
                    // For now, we silently drop it and return an empty description
                    // to the sender to prevent from leaking any privacy/security-relevant
                    // data to the other side.
                    reply = core::dbus::Message::make_error(
                                in,
                                Errors::CouldNotRegisterAgentForUser::name,
                                "");
                } catch(...)
                {
                    reply = core::dbus::Message::make_error(
                                in,
                                Errors::CouldNotRegisterAgentForUser::name,
                                "");
                }

                try
                {
                    configuration.bus->send(reply);
                } catch(const std::exception&)
                {
                    // TODO(tvoss): We should report the exception here.
                    // We immediately remove the agent for the given user id
                    // as the transaction to the other side did not complete.
                    unregister_agent_for_user(uid);
                } catch(...)
                {
                    // TODO(tvoss): We should report an issue here.
                    // We immediately remove the agent for the given user id
                    // as the transaction to the other side did not complete.
                    unregister_agent_for_user(uid);
                }
            });

            configuration.object->install_method_handler<Methods::UnregisterAgentForUser>([this](const core::dbus::Message::Ptr& in)
            {
                core::trust::Uid uid; in->reader() >> uid;

                try
                {
                    configuration.impl.unregister_agent_for_user(uid);
                } catch(const std::exception&)
                {
                    // Just dropping the exception here.
                    // We should definitely report any issues here, but
                    // stay silent for now.
                }

                configuration.bus->send(core::dbus::Message::make_method_return(in));
            });
        }

        // Uninstalls method handlers.
        ~Skeleton()
        {
            configuration.object->uninstall_method_handler<Methods::RegisterAgentForUser>();
            configuration.object->uninstall_method_handler<Methods::UnregisterAgentForUser>();
        }

        // Registers an agent for the given uid,
        // calls out to the implementation given at construction time.
        void register_agent_for_user(const core::trust::Uid& uid, const std::shared_ptr<core::trust::Agent>& agent) override
        {
            configuration.impl.register_agent_for_user(uid, agent);
        }

        // Removes the agent for the given uid from the registry,
        // calls out to the implementation given at construction time.
        void unregister_agent_for_user(const core::trust::Uid& uid) override
        {
            configuration.impl.unregister_agent_for_user(uid);
        }

        // We just store all creation time parameters
        Configuration configuration;
    };
};
}
}
}

#endif // CORE_TRUST_DBUS_AGENT_REGISTRY_H_
