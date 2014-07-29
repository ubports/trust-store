/*
 * Copyright © 2013 Canonical Ltd.
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
#include <core/trust/dbus/agent_registry.h>

#include "mock_agent.h"

#include <core/dbus/fixture.h>
#include <core/dbus/asio/executor.h>

#include <core/posix/fork.h>

#include <core/testing/cross_process_sync.h>
#include <core/testing/fork_and_run.h>

#include <gtest/gtest.h>

namespace
{
std::shared_ptr<core::posix::SignalTrap> a_trap_for_sig_term()
{
    auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
    trap->signal_raised().connect([trap](core::posix::Signal)
    {
        trap->stop();
    });

    return trap;
}

struct DBusAgent : public core::dbus::testing::Fixture
{
    static constexpr const char* agent_service_name
    {
        "core.trust.dbus.Agent"
    };

    static constexpr const char* agent_path
    {
        "/core/trust/dbus/Agent"
    };
};

struct DBusAgentRegistry : public core::dbus::testing::Fixture
{
    static constexpr const char* agent_registry_service_name
    {
        "core.trust.dbus.AgentRegistry"
    };

    static constexpr const char* agent_registry_path
    {
        "/core/trust/dbus/AgentRegistry"
    };
};

struct MockAgentRegistry : public core::trust::Agent::Registry
{
    // Registers an agent for the given uid.
    MOCK_METHOD2(register_agent_for_user, void(const core::trust::Uid&, const std::shared_ptr<core::trust::Agent>&));

    // Removes the agent for the given uid from the registry
    MOCK_METHOD1(unregister_agent_for_user, void(const core::trust::Uid&));
};

core::trust::Agent::RequestParameters ref_params
{
    core::trust::Uid{42},
    core::trust::Pid{41},
    "just.a.testing.id",
    core::trust::Feature{40},
    "just an example description"
};

}

TEST_F(DBusAgent, remote_invocation_works_correctly)
{
    using namespace ::testing;

    core::testing::CrossProcessSync cps;

    auto skeleton = [this, &cps]()
    {
        auto trap = a_trap_for_sig_term();

        auto agent = std::make_shared<MockAgent>();

        EXPECT_CALL(*agent, authenticate_request_with_parameters(_))
                .Times(1)
                .WillRepeatedly(Return(core::trust::Request::Answer::denied));

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        std::thread t{[bus]() { bus->run(); }};

        auto service = core::dbus::Service::add_service(bus, agent_service_name);
        auto object = service->add_object_for_path(core::dbus::types::ObjectPath{agent_path});

        core::trust::dbus::Agent::Skeleton skeleton
        {
            core::trust::dbus::Agent::Skeleton::Configuration
            {
                object,
                bus,
                [agent](const core::trust::Agent::RequestParameters& params)
                {
                    return agent->authenticate_request_with_parameters(params);
                }
            }
        };

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        bus->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    };

    auto stub = [this, &cps]()
    {
        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto service = core::dbus::Service::use_service(bus, agent_service_name);
        auto object = service->object_for_path(core::dbus::types::ObjectPath{agent_path});

        core::trust::dbus::Agent::Stub stub
        {
            object
        };

        std::thread t{[bus]() { bus->run(); }};

        EXPECT_EQ(core::trust::Request::Answer::denied,
                  stub.authenticate_request_with_parameters(ref_params));

        bus->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty,
              core::testing::fork_and_run(skeleton, stub));
}

TEST_F(DBusAgent, remote_invocation_throws_if_answer_non_conclusive)
{
    using namespace ::testing;

    core::testing::CrossProcessSync cps;

    auto skeleton = [this, &cps]()
    {
        auto trap = a_trap_for_sig_term();

        auto agent = std::make_shared<MockAgent>();

        EXPECT_CALL(*agent, authenticate_request_with_parameters(_))
                .Times(1)
                .WillRepeatedly(Throw(std::logic_error{""}));

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        std::thread t{[bus]() { bus->run(); }};

        auto service = core::dbus::Service::add_service(bus, agent_service_name);
        auto object = service->add_object_for_path(core::dbus::types::ObjectPath{agent_path});

        core::trust::dbus::Agent::Skeleton skeleton
        {
            core::trust::dbus::Agent::Skeleton::Configuration
            {
                object,
                bus,
                [agent](const core::trust::Agent::RequestParameters& params)
                {
                    return agent->authenticate_request_with_parameters(params);
                }
            }
        };

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        bus->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    };

    auto stub = [this, &cps]()
    {
        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto service = core::dbus::Service::use_service(bus, agent_service_name);
        auto object = service->object_for_path(core::dbus::types::ObjectPath{agent_path});

        core::trust::dbus::Agent::Stub stub
        {
            object
        };

        std::thread t{[bus]() { bus->run(); }};

        EXPECT_ANY_THROW(stub.authenticate_request_with_parameters(ref_params));

        bus->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty,
              core::testing::fork_and_run(skeleton, stub));
}

TEST_F(DBusAgentRegistry, remote_invocation_works_correctly)
{
    using namespace ::testing;

    static const int expected_invocation_count{10};

    core::testing::CrossProcessSync service_ready, agent_registered;

    auto skeleton = [this, &service_ready, &agent_registered]()
    {
        auto trap = a_trap_for_sig_term();

        core::trust::LockingAgentRegistry locking_agent_registry;

        MockAgentRegistry agent_registry;

        EXPECT_CALL(agent_registry, register_agent_for_user(_, _))
                .Times(1)
                .WillRepeatedly(Invoke(&locking_agent_registry, &core::trust::LockingAgentRegistry::register_agent_for_user));

        EXPECT_CALL(agent_registry, unregister_agent_for_user(_))
                .Times(1);

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        std::thread t{[bus]() { bus->run(); }};
        std::thread u{[&locking_agent_registry, &agent_registered]()
        {
            agent_registered.wait_for_signal_ready_for(std::chrono::seconds{5});

            auto agent = locking_agent_registry.agent_for_user(core::trust::Uid{::getuid()});

            for (unsigned int i = 0; i < expected_invocation_count; i++)
                EXPECT_EQ(core::trust::Request::Answer::denied,
                          agent->authenticate_request_with_parameters(ref_params));
        }};

        auto service = core::dbus::Service::add_service(bus, agent_registry_service_name);
        auto object = service->add_object_for_path(core::dbus::types::ObjectPath{agent_registry_path});

        core::trust::dbus::AgentRegistry::Skeleton skeleton
        {
            core::trust::dbus::AgentRegistry::Skeleton::Configuration
            {
                object,
                bus,
                agent_registry
            }
        };

        service_ready.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        bus->stop();

        if (t.joinable())
            t.join();

        if (u.joinable())
            u.join();

        return ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    };

    auto stub = [this, &service_ready, &agent_registered]()
    {
        service_ready.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto service = core::dbus::Service::use_service(bus, agent_registry_service_name);
        auto agent_object = service->add_object_for_path(core::dbus::types::ObjectPath{DBusAgent::agent_path});
        auto agent_registry_object = service->object_for_path(core::dbus::types::ObjectPath{DBusAgentRegistry::agent_registry_path});

        struct State
        {
            void wait()
            {
                std::unique_lock<std::mutex> ul(guard);
                wait_condition.wait_for(
                            ul,
                            std::chrono::milliseconds{1000},
                            [this]() { return invocation_count == expected_invocation_count; });
            }

            void notify()
            {
                invocation_count++;
                wait_condition.notify_all();
            }

            std::uint32_t invocation_count{0};
            std::mutex guard;
            std::condition_variable wait_condition;
        } state;

        auto agent = std::make_shared<MockAgent>();

        EXPECT_CALL(*agent, authenticate_request_with_parameters(ref_params))
                .Times(expected_invocation_count)
                .WillRepeatedly(
                    DoAll(
                        InvokeWithoutArgs(&state, &State::notify),
                        Return(core::trust::Request::Answer::denied)));

        /*std::shared_ptr<core::trust::dbus::Agent::Skeleton> skeleton
        {
            new core::trust::dbus::Agent::Skeleton
            {
                core::trust::dbus::Agent::Skeleton::Configuration
                {
                    agent_object,
                    bus,
                    [agent](const core::trust::Agent::RequestParameters& params)
                    {
                        return agent->authenticate_request_with_parameters(params);
                    }
                }
            }
        };*/

        core::trust::dbus::AgentRegistry::Stub stub
        {
            core::trust::dbus::AgentRegistry::Stub::Configuration
            {
                agent_registry_object,
                core::trust::dbus::AgentRegistry::Stub::counting_object_path_generator(),
                service,
                bus
            }
        };

        std::thread t{[bus]() { bus->run(); }};

        // We register for the current user id.
        stub.register_agent_for_user(core::trust::Uid{::getuid()}, agent);

        // Tell the other side that we are good to go.
        agent_registered.try_signal_ready_for(std::chrono::milliseconds{500});

        // We wait until we have seen 5 invocations
        state.wait();

        // And unregister again.
        stub.unregister_agent_for_user(core::trust::Uid{::getuid()});

        bus->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty,
              core::testing::fork_and_run(skeleton, stub));
}
