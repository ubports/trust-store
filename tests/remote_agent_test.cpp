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

// Implementation-specific header
#include <core/trust/remote/agent.h>
#include <core/trust/remote/dbus.h>
#include <core/trust/remote/posix.h>

#include "mock_agent.h"
#include "process_exited_successfully.h"

#include <core/posix/fork.h>
#include <core/testing/cross_process_sync.h>

#include <core/dbus/asio/executor.h>
#include <core/dbus/fixture.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

namespace
{
struct MockRemoteAgentStub : public core::trust::remote::Agent::Stub
{
    // Sends out the request to the receiving end, either returning an answer
    // or throwing an exception if no conclusive answer could be obtained from
    // the user.
    MOCK_METHOD1(send, core::trust::Request::Answer(const core::trust::Agent::RequestParameters&));
};

}

TEST(RemoteAgentStub, calls_send_for_handling_requests_and_returns_answer)
{
    using namespace ::testing;

    core::trust::Agent::RequestParameters parameters
    {
        core::trust::Uid{21},
        core::trust::Pid{42},
        std::string{"does.not.exist"},
        core::trust::Feature{},
        std::string{"some meaningless description"}
    };

    MockRemoteAgentStub stub;
    EXPECT_CALL(stub, send(parameters))
            .Times(1)
            .WillOnce(Return(core::trust::Request::Answer::granted));

    EXPECT_EQ(core::trust::Request::Answer::granted,
              stub.authenticate_request_with_parameters(parameters));
}

TEST(RemoteAgentSkeleton, calls_out_to_implementation)
{
    using namespace ::testing;

    core::trust::Agent::RequestParameters parameters
    {
        core::trust::Uid{21},
        core::trust::Pid{42},
        std::string{"does.not.exist"},
        core::trust::Feature{},
        std::string{"some meaningless description"}
    };

    std::shared_ptr<MockAgent> mock_agent
    {
        new MockAgent{}
    };

    core::trust::remote::Agent::Skeleton skeleton
    {
        mock_agent
    };

    EXPECT_CALL(*mock_agent, authenticate_request_with_parameters(parameters))
            .Times(1)
            .WillOnce(Return(core::trust::Request::Answer::granted));

    EXPECT_EQ(core::trust::Request::Answer::granted,
              skeleton.authenticate_request_with_parameters(parameters));
}

TEST(RemoteAgentStubSessionRegistry, adding_and_removing_of_a_valid_session_works)
{
    using Session = core::trust::remote::posix::Stub::Session;

    boost::asio::io_service io_service;

    Session::Ptr session
    {
        new Session
        {
            io_service
        }
    };

    Session::Registry::Ptr registry
    {
        new Session::Registry{}
    };

    core::trust::Uid uid{::getuid()};

    EXPECT_NO_THROW(registry->add_session_for_uid(uid, session););
    EXPECT_TRUE(registry->has_session_for_uid(uid));
    EXPECT_NE(nullptr, registry->resolve_session_for_uid(uid));
    EXPECT_NO_THROW(registry->remove_session_for_uid(uid););
    EXPECT_FALSE(registry->has_session_for_uid(uid));
    EXPECT_THROW(registry->resolve_session_for_uid(uid), std::out_of_range);
}

TEST(RemoteAgentStubSessionRegistry, adding_a_null_session_throws)
{
    using Session = core::trust::remote::posix::Stub::Session;

    Session::Ptr session;

    Session::Registry::Ptr registry
    {
        new Session::Registry{}
    };

    core::trust::Uid uid{::getuid()};

    EXPECT_THROW(registry->add_session_for_uid(uid, session), std::logic_error);
    EXPECT_FALSE(registry->has_session_for_uid(uid));
    EXPECT_THROW(registry->resolve_session_for_uid(uid), std::out_of_range);
}

TEST(RemoteAgentStubSessionRegistry, resolving_a_non_existing_session_throws)
{
    using Session = core::trust::remote::posix::Stub::Session;

    Session::Registry::Ptr registry
    {
        new Session::Registry{}
    };

    core::trust::Uid uid{::getuid()};

    EXPECT_THROW(registry->resolve_session_for_uid(uid), std::out_of_range);
}

TEST(RemoteAgentStubSessionRegistry, removing_a_non_existing_session_does_not_throw)
{
    using Session = core::trust::remote::posix::Stub::Session;

    Session::Registry::Ptr registry
    {
        new Session::Registry{}
    };

    core::trust::Uid uid{::getuid()};

    EXPECT_FALSE(registry->has_session_for_uid(uid));
    EXPECT_NO_THROW(registry->remove_session_for_uid(uid));
}


namespace
{
struct UnixDomainSocketRemoteAgent : public ::testing::Test
{
    static constexpr const char* endpoint_for_testing
    {
        "/tmp/unlikely.to.ever.be.used.by.someone.else"
    };

    UnixDomainSocketRemoteAgent()
        : keep_alive{io_service},
          worker{[this]() { io_service.run(); }}
    {
        std::remove(endpoint_for_testing);
    }

    ~UnixDomainSocketRemoteAgent()
    {
        io_service.stop();

        if (worker.joinable())
            worker.join();
    }

    // Returns the default stub setup for testing
    core::trust::remote::posix::Stub::Configuration the_default_stub_configuration()
    {
        return core::trust::remote::posix::Stub::Configuration
        {
            io_service,
            boost::asio::local::stream_protocol::endpoint{UnixDomainSocketRemoteAgent::endpoint_for_testing},
            core::trust::remote::helpers::proc_stat_start_time_resolver(),
            core::trust::remote::posix::Stub::get_sock_opt_credentials_resolver(),
            std::make_shared<core::trust::remote::posix::Stub::Session::Registry>()
        };
    }

    // Returns a peer ready to be forked that connects to the endpoint_for_testing
    // and immediately exits, checking for any sort of testing failures that might have
    // occured in between.
    static std::function<core::posix::exit::Status()> a_raw_peer_immediately_exiting()
    {
        return []()
        {
            boost::asio::io_service io_service;
            boost::asio::io_service::work keep_alive{io_service};

            std::thread worker{[&io_service]() { io_service.run(); }};

            boost::asio::local::stream_protocol::socket socket{io_service};

            EXPECT_NO_THROW(socket.connect(boost::asio::local::stream_protocol::endpoint{UnixDomainSocketRemoteAgent::endpoint_for_testing}));

            io_service.stop();
            if(worker.joinable())
                worker.join();

            return ::testing::Test::HasFailure() ?
                        core::posix::exit::Status::failure :
                        core::posix::exit::Status::success;
        };
    }



    boost::asio::io_service io_service;
    boost::asio::io_service::work keep_alive;

    std::thread worker;
};

struct MockProcessStartTimeResolver
{
    virtual ~MockProcessStartTimeResolver() = default;

    core::trust::remote::helpers::ProcessStartTimeResolver to_functional()
    {
        return [this](core::trust::Pid pid)
        {
            return resolve_process_start_time(pid);
        };
    }

    MOCK_METHOD1(resolve_process_start_time, std::int64_t(core::trust::Pid));
};

}

TEST_F(UnixDomainSocketRemoteAgent, accepts_connections_on_endpoint)
{
    auto stub = core::trust::remote::posix::Stub::create_stub_for_configuration(
                the_default_stub_configuration());

    // Make sure that the stub and all its threads are spun up.
    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    core::posix::ChildProcess child = core::posix::fork(
                UnixDomainSocketRemoteAgent::a_raw_peer_immediately_exiting(),
                core::posix::StandardStream::empty);

    EXPECT_TRUE(ProcessExitedSuccessfully(child.wait_for(core::posix::wait::Flags::untraced)));
    EXPECT_TRUE(stub->has_session_for_uid(core::trust::Uid{::getuid()}));
}

TEST_F(UnixDomainSocketRemoteAgent, queries_peer_credentials_for_incoming_connection)
{
    static const core::trust::Uid uid{42};
    bool peer_credentials_queried{false};

    auto config = the_default_stub_configuration();

    config.peer_credentials_resolver = [&peer_credentials_queried](int)
    {
        peer_credentials_queried = true;
        return std::make_tuple(uid, core::trust::Pid{42}, core::trust::Gid{42});
    };

    auto stub = core::trust::remote::posix::Stub::create_stub_for_configuration(config);

    core::posix::ChildProcess child = core::posix::fork(
                UnixDomainSocketRemoteAgent::a_raw_peer_immediately_exiting(),
                core::posix::StandardStream::empty);

    EXPECT_TRUE(ProcessExitedSuccessfully(child.wait_for(core::posix::wait::Flags::untraced)));

    EXPECT_TRUE(peer_credentials_queried);
    EXPECT_TRUE(stub->has_session_for_uid(uid));
}

TEST_F(UnixDomainSocketRemoteAgent, stub_and_skeleton_query_process_start_time_for_request)
{
    using namespace ::testing;

    // We need to make sure that we have a valid pid.
    auto app = core::posix::fork([]()
    {
        while(true) std::this_thread::sleep_for(std::chrono::milliseconds{500});
        return core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    // skeleton --| good to go |--> stub
    core::testing::CrossProcessSync cps;

    NiceMock<MockProcessStartTimeResolver> process_start_time_resolver;

    auto config = the_default_stub_configuration();
    config.start_time_resolver = process_start_time_resolver.to_functional();

    auto stub = core::trust::remote::posix::Stub::create_stub_for_configuration(config);

    core::posix::ChildProcess child = core::posix::fork([&cps]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});

        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        boost::asio::io_service io_service;
        boost::asio::io_service::work work{io_service};
        std::thread worker{[&io_service] { io_service.run(); }};

        NiceMock<MockProcessStartTimeResolver> process_start_time_resolver;

        EXPECT_CALL(process_start_time_resolver, resolve_process_start_time(_))
                .Times(1)
                .WillRepeatedly(Return(42));

        auto mock_agent = std::make_shared<::testing::NiceMock<MockAgent>>();

        EXPECT_CALL(*mock_agent, authenticate_request_with_parameters(_))
                .Times(1)
                .WillRepeatedly(Return(core::trust::Request::Answer::denied));

        core::trust::remote::posix::Skeleton::Configuration config
        {
            mock_agent,
            io_service,
            boost::asio::local::stream_protocol::endpoint{UnixDomainSocketRemoteAgent::endpoint_for_testing},
            process_start_time_resolver.to_functional(),
            core::trust::remote::helpers::aa_get_task_con_app_id_resolver(),
            "Just a test for %1%.",
            true
        };

        auto skeleton = core::trust::remote::posix::Skeleton::create_skeleton_for_configuration(config);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        io_service.stop();

        if (worker.joinable())
            worker.join();

        return ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

    EXPECT_CALL(process_start_time_resolver, resolve_process_start_time(_))
            .Times(2)
            .WillRepeatedly(Return(42));

    EXPECT_EQ(core::trust::Request::Answer::denied,
              stub->authenticate_request_with_parameters(
                  core::trust::Agent::RequestParameters
                  {
                      core::trust::Uid{::getuid()},
                      core::trust::Pid{app.pid()},
                      "",
                      core::trust::Feature{},
                      ""
                  }));

    child.send_signal_or_throw(core::posix::Signal::sig_term);
    EXPECT_TRUE(ProcessExitedSuccessfully(child.wait_for(core::posix::wait::Flags::untraced)));
}

/**************************************
  Full blown acceptance tests go here.
**************************************/

#include <ctime>

namespace
{
static constexpr const char* endpoint_for_acceptance_testing
{
    "/tmp/endpoint.for.acceptance.testing"
};

// Our rng used in acceptance testing, seeded with the current time since
// the epoch in seconds.
static std::default_random_engine rng
{
    static_cast<std::default_random_engine::result_type>(std::time(nullptr))
};

// Our dice
static std::uniform_int_distribution<int> dice(1,6);
}

TEST(UnixDomainSocket, a_service_can_query_a_remote_agent)
{
    using namespace ::testing;

    std::remove(endpoint_for_acceptance_testing);

    core::testing::CrossProcessSync
            stub_ready,         // signals stub     --| I'm ready |--> skeleton
            skeleton_ready;     // signals skeleton --| I'm ready |--> stub

    auto app = core::posix::fork([]()
    {
        while(true) std::this_thread::sleep_for(std::chrono::milliseconds{500});
        return core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    // We sample an answer by throwing a dice.
    const core::trust::Request::Answer answer
    {
        dice(rng) <= 3 ?
                    core::trust::Request::Answer::denied :
                    core::trust::Request::Answer::granted
    };

    const core::trust::Uid app_uid{::getuid()};
    const core::trust::Pid app_pid{app.pid()};

    auto stub = core::posix::fork([app_uid, app_pid, answer, &stub_ready, &skeleton_ready]()
    {
        boost::asio::io_service io_service;
        boost::asio::io_service::work keep_alive{io_service};

        std::thread worker{[&io_service]() { io_service.run(); }};

        core::trust::remote::posix::Stub::Configuration config
        {
            io_service,
            boost::asio::local::stream_protocol::endpoint{endpoint_for_acceptance_testing},
            core::trust::remote::helpers::proc_stat_start_time_resolver(),
            core::trust::remote::posix::Stub::get_sock_opt_credentials_resolver(),
            std::make_shared<core::trust::remote::posix::Stub::Session::Registry>()
        };

        auto stub = core::trust::remote::posix::Stub::create_stub_for_configuration(config);

        std::this_thread::sleep_for(std::chrono::milliseconds{500});

        stub_ready.try_signal_ready_for(std::chrono::milliseconds{1000});
        skeleton_ready.wait_for_signal_ready_for(std::chrono::milliseconds{1000});

        for (unsigned int i = 0; i < 100; i++)
        {
            EXPECT_EQ(answer, stub->authenticate_request_with_parameters(
                          core::trust::Agent::RequestParameters
                          {
                              app_uid,
                              app_pid,
                              "",
                              core::trust::Feature{},
                              ""
                          }));
        }

        io_service.stop();

        if (worker.joinable())
            worker.join();

        return Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    auto skeleton = core::posix::fork([answer, &stub_ready, &skeleton_ready]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});

        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        boost::asio::io_service io_service;
        boost::asio::io_service::work keep_alive{io_service};

        std::thread worker{[&io_service]() { io_service.run(); }};

        // We have to rely on a MockAgent to break the dependency on a running Mir instance.
        auto mock_agent = std::make_shared<::testing::NiceMock<MockAgent>>();

        ON_CALL(*mock_agent, authenticate_request_with_parameters(_))
                .WillByDefault(Return(answer));

        core::trust::remote::posix::Skeleton::Configuration config
        {
            mock_agent,
            io_service,
            boost::asio::local::stream_protocol::endpoint{endpoint_for_acceptance_testing},
            core::trust::remote::helpers::proc_stat_start_time_resolver(),
            core::trust::remote::helpers::aa_get_task_con_app_id_resolver(),
            "Just a test for %1%.",
            true
        };

        stub_ready.wait_for_signal_ready_for(std::chrono::milliseconds{1000});

        auto skeleton = core::trust::remote::posix::Skeleton::create_skeleton_for_configuration(config);
        std::this_thread::sleep_for(std::chrono::milliseconds{500});

        skeleton_ready.try_signal_ready_for(std::chrono::milliseconds{1000});

        trap->run();

        io_service.stop();

        if (worker.joinable())
            worker.join();

        return core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    EXPECT_TRUE(ProcessExitedSuccessfully(stub.wait_for(core::posix::wait::Flags::untraced)));
    skeleton.send_signal_or_throw(core::posix::Signal::sig_term);
    EXPECT_TRUE(ProcessExitedSuccessfully(skeleton.wait_for(core::posix::wait::Flags::untraced)));

    app.send_signal_or_throw(core::posix::Signal::sig_kill);
}

// A test case using a standalone service that is not using our header files.
// The test-case here is equivalent to the code that we are wiring up in the
// Android camera service.

#include <sys/socket.h>
#include <sys/un.h>

namespace
{
// We declare another struct for testing purposes.
struct Request
{
    uid_t uid; pid_t pid; ::uint64_t feature; ::int64_t start_time;
};
}

TEST(UnixDomainSocket, a_standalone_service_can_query_a_remote_agent)
{
    using namespace ::testing;

    std::remove(endpoint_for_acceptance_testing);

    core::testing::CrossProcessSync
            stub_ready,         // signals stub     --| I'm ready |--> skeleton
            skeleton_ready;     // signals skeleton --| I'm ready |--> stub

    auto app = core::posix::fork([]()
    {
        while(true) std::this_thread::sleep_for(std::chrono::milliseconds{500});
        return core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    // We sample an answer by throwing a dice.
    const core::trust::Request::Answer answer
    {
        dice(rng) <= 3 ?
                    core::trust::Request::Answer::denied :
                    core::trust::Request::Answer::granted
    };

    const core::trust::Uid app_uid{::getuid()};
    const core::trust::Pid app_pid{app.pid()};

    auto start_time_helper = core::trust::remote::helpers::proc_stat_start_time_resolver();
    auto app_start_time = start_time_helper(core::trust::Pid{app.pid()});

    auto stub = core::posix::fork([app_uid, app_pid, app_start_time, answer, &stub_ready, &skeleton_ready]()
    {
        // Stores all known connections indexed by user id
        std::map<uid_t, int> known_connections_by_user_id;

        static const int socket_error_code{-1};

        // We create a unix domain socket
        int socket_fd = ::socket(PF_UNIX, SOCK_STREAM, 0);

        if (socket_fd == socket_error_code)
        {
            ::perror("Could not create unix stream socket");
            return core::posix::exit::Status::failure;
        }

        // Prepare for binding to endpoint in file system.
        // Consciously ignoring errors here.
        ::unlink(endpoint_for_acceptance_testing);

        // Setup address
        sockaddr_un address;
        ::memset(&address, 0, sizeof(sockaddr_un));

        address.sun_family = AF_UNIX;
        ::snprintf(address.sun_path, 108, endpoint_for_acceptance_testing);

        // And bind to the endpoint in the filesystem.
        static const int bind_error_code{-1};
        int rc = ::bind(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr_un));

        if (rc == bind_error_code)
        {
            ::perror("Could not bind to endpoint");
            return core::posix::exit::Status::failure;
        }

        // Start listening for incoming connections.
        static const int listen_error_code{-1};
        static const int back_log_size{5};

        rc = ::listen(socket_fd, back_log_size);

        if (rc == listen_error_code)
        {
            ::perror("Could not start listening for incoming connections");
            return core::posix::exit::Status::failure;
        }

        // Spawn a thread for accepting connections.
        std::thread acceptor([socket_fd, &known_connections_by_user_id]()
        {
            bool keep_on_running{true};
            // Error code when accepting connections.
            static const int accept_error_code{-1};

            // Error code when querying socket options.
            static const int get_sock_opt_error = -1;
            // Some state we preserve across loop iterations.
            sockaddr_un address;
            socklen_t address_length = sizeof(sockaddr_un);

            ucred peer_credentials { 0, 0, 0 };
            socklen_t len = sizeof(peer_credentials);

            while(keep_on_running)
            {
                address_length = sizeof(sockaddr_un);
                int connection_fd = ::accept(socket_fd, reinterpret_cast<sockaddr*>(&address), &address_length);

                if (connection_fd == accept_error_code)
                {
                    switch (errno)
                    {
                    case EINTR:
                    case EAGAIN:
                        keep_on_running = true;
                        break;
                    default:
                        keep_on_running = false;
                        break;
                    }
                } else
                {
                    // We query the peer credentials
                    len = sizeof(ucred);
                    auto rc = ::getsockopt(connection_fd, SOL_SOCKET, SO_PEERCRED, &peer_credentials, &len);

                    if (rc == get_sock_opt_error)
                    {
                        ::perror("Could not query peer credentials");
                    } else
                    {
                        known_connections_by_user_id.insert(
                                    std::make_pair(
                                        peer_credentials.uid,
                                        connection_fd));
                    }
                }
            }
        });

        // This is obviously super ugly, but we would like to avoid implementing
        // a full blown reactor pattern.
        acceptor.detach();

        stub_ready.try_signal_ready_for(std::chrono::milliseconds{1000});
        skeleton_ready.wait_for_signal_ready_for(std::chrono::milliseconds{1000});

        std::this_thread::sleep_for(std::chrono::milliseconds{500});

        Request request
        {
            app_uid.value,
            app_pid.value,
            0,
            app_start_time
        };

        for (unsigned int i = 0; i < 100; i++)
        {
            std::int32_t answer_from_socket;

            int socket = known_connections_by_user_id.at(::getuid());

            EXPECT_NE(-1, ::write(socket, &request, sizeof(Request)));
            EXPECT_NE(-1, ::read(socket, &answer_from_socket, sizeof(std::int32_t)));

            EXPECT_EQ(static_cast<std::int32_t>(answer), answer_from_socket);
        }

        ::close(socket_fd);

        return Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    auto skeleton = core::posix::fork([answer, &stub_ready, &skeleton_ready]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});

        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        boost::asio::io_service io_service;
        boost::asio::io_service::work keep_alive{io_service};

        std::thread worker{[&io_service]() { io_service.run(); }};

        // We have to rely on a MockAgent to break the dependency on a running Mir instance.
        auto mock_agent = std::make_shared<::testing::NiceMock<MockAgent>>();

        ON_CALL(*mock_agent, authenticate_request_with_parameters(_))
                .WillByDefault(Return(answer));

        core::trust::remote::posix::Skeleton::Configuration config
        {
            mock_agent,
            io_service,
            boost::asio::local::stream_protocol::endpoint{endpoint_for_acceptance_testing},
            core::trust::remote::helpers::proc_stat_start_time_resolver(),
            core::trust::remote::helpers::aa_get_task_con_app_id_resolver(),
            "Just a test for %1%.",
            true
        };

        stub_ready.wait_for_signal_ready_for(std::chrono::milliseconds{1000});
        auto skeleton = core::trust::remote::posix::Skeleton::create_skeleton_for_configuration(config);
        skeleton_ready.try_signal_ready_for(std::chrono::milliseconds{1000});

        trap->run();

        io_service.stop();

        if (worker.joinable())
            worker.join();

        return core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    EXPECT_TRUE(ProcessExitedSuccessfully(stub.wait_for(core::posix::wait::Flags::untraced)));
    skeleton.send_signal_or_throw(core::posix::Signal::sig_term);
    EXPECT_TRUE(ProcessExitedSuccessfully(skeleton.wait_for(core::posix::wait::Flags::untraced)));

    app.send_signal_or_throw(core::posix::Signal::sig_kill);
}

namespace
{
struct DBus : public core::dbus::testing::Fixture
{
};

std::string service_name
{
    "JustForTestingPurposes"
};
}
// A test-case using the remote dbus agent implementation.
TEST_F(DBus, a_service_can_query_a_remote_agent)
{
    using namespace ::testing;

    core::testing::CrossProcessSync
            stub_ready,         // signals stub     --| I'm ready |--> skeleton
            skeleton_ready;     // signals skeleton --| I'm ready |--> stub

    auto app = core::posix::fork([]()
    {
        while(true) std::this_thread::sleep_for(std::chrono::milliseconds{500});
        return core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    // We sample an answer by throwing a dice.
    const core::trust::Request::Answer answer
    {
        dice(rng) <= 3 ?
                    core::trust::Request::Answer::denied :
                    core::trust::Request::Answer::granted
    };

    const core::trust::Uid app_uid{::getuid()};
    const core::trust::Pid app_pid{app.pid()};

    auto stub = core::posix::fork([this, app_uid, app_pid, answer, &stub_ready, &skeleton_ready]()
    {
        core::trust::Agent::RequestParameters ref_params
        {
            app_uid,
            app_pid,
            "just.a.testing.id",
            core::trust::Feature{40},
            "just an example description"
        };

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        std::thread worker{[bus]() { bus->run(); }};

        std::string dbus_service_name = core::trust::remote::dbus::default_service_name_prefix + std::string{"."} + service_name;

        auto service = core::dbus::Service::add_service(bus, dbus_service_name);
        auto object = service->add_object_for_path(core::dbus::types::ObjectPath
        {
            core::trust::remote::dbus::default_agent_registry_path
        });

        core::trust::remote::dbus::Agent::Stub::Configuration config
        {
            object,
            bus
        };

        auto stub = std::make_shared<core::trust::remote::dbus::Agent::Stub>(config);

        stub_ready.try_signal_ready_for(std::chrono::milliseconds{1000});
        skeleton_ready.wait_for_signal_ready_for(std::chrono::milliseconds{1000});

        std::this_thread::sleep_for(std::chrono::seconds{1});

        for (unsigned int i = 0; i < 100; i++)
            EXPECT_EQ(answer, stub->authenticate_request_with_parameters(ref_params));

        bus->stop();

        if (worker.joinable())
            worker.join();

        return Test::HasFailure() ?
                    core::posix::exit::Status::failure :
                    core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    auto skeleton = core::posix::fork([this, answer, &stub_ready, &skeleton_ready]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});

        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        std::thread worker{[bus]() { bus->run(); }};

        // We have to rely on a MockAgent to break the dependency on a running Mir instance.
        auto mock_agent = std::make_shared<::testing::NiceMock<MockAgent>>();

        ON_CALL(*mock_agent, authenticate_request_with_parameters(_))
                .WillByDefault(Return(answer));

        Mock::AllowLeak(mock_agent.get());

        std::string dbus_service_name = core::trust::remote::dbus::default_service_name_prefix + std::string{"."} + service_name;

        stub_ready.wait_for_signal_ready_for(std::chrono::milliseconds{1000});

        auto service = core::dbus::Service::use_service(bus, dbus_service_name);
        auto object = service->object_for_path(core::dbus::types::ObjectPath
        {
            core::trust::remote::dbus::default_agent_registry_path
        });

        core::dbus::DBus daemon{bus};

        core::trust::remote::dbus::Agent::Skeleton::Configuration config
        {
            mock_agent,
            object,
            daemon.make_service_watcher(dbus_service_name),
            service,
            bus,
            core::trust::remote::helpers::aa_get_task_con_app_id_resolver()
        };

        auto skeleton = std::make_shared<core::trust::remote::dbus::Agent::Skeleton>(std::move(config));

        skeleton_ready.try_signal_ready_for(std::chrono::milliseconds{1000});

        trap->run();

        bus->stop();

        if (worker.joinable())
            worker.join();

        return core::posix::exit::Status::success;
    }, core::posix::StandardStream::empty);

    EXPECT_TRUE(ProcessExitedSuccessfully(stub.wait_for(core::posix::wait::Flags::untraced)));
    skeleton.send_signal_or_throw(core::posix::Signal::sig_term);
    EXPECT_TRUE(ProcessExitedSuccessfully(skeleton.wait_for(core::posix::wait::Flags::untraced)));

    app.send_signal_or_throw(core::posix::Signal::sig_kill);
}
