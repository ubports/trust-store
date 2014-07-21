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
#include <core/trust/remote/unix_domain_socket_agent.h>

#include "mock_agent.h"

#include <core/posix/fork.h>
#include <core/testing/cross_process_sync.h>

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
    MOCK_METHOD4(send, core::trust::Request::Answer(uid_t, pid_t, const std::string&, const std::string&));
};
}

TEST(RemoteAgentStub, calls_send_for_handling_requests_and_returns_answer)
{
    using namespace ::testing;

    uid_t app_uid{21};
    pid_t app_pid{42};
    std::string app_id{"does.not.exist"};
    std::string description{"some meaningless description"};

    MockRemoteAgentStub stub;
    EXPECT_CALL(stub, send(app_uid, app_pid, app_id, description))
            .Times(1)
            .WillOnce(Return(core::trust::Request::Answer::granted));

    EXPECT_EQ(core::trust::Request::Answer::granted,
              stub.prompt_user_for_request(app_uid, app_pid, app_id, description));
}

TEST(RemoteAgentSkeleton, calls_out_to_implementation)
{
    using namespace ::testing;

    uid_t app_uid{21};
    pid_t app_pid{42};
    std::string app_id{"does.not.exist"};
    std::string description{"some meaningless description"};

    std::shared_ptr<MockAgent> mock_agent
    {
        new MockAgent{}
    };

    core::trust::remote::Agent::Skeleton skeleton
    {
        mock_agent
    };

    EXPECT_CALL(*mock_agent, prompt_user_for_request(app_uid, app_pid, app_id, description))
            .Times(1)
            .WillOnce(Return(core::trust::Request::Answer::granted));

    EXPECT_EQ(core::trust::Request::Answer::granted,
              skeleton.prompt_user_for_request(app_uid, app_pid, app_id, description));
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
    core::trust::remote::UnixDomainSocketAgent::Stub::Configuration the_default_stub_configuration()
    {
        return core::trust::remote::UnixDomainSocketAgent::Stub::Configuration
        {
            io_service,
            boost::asio::local::stream_protocol::endpoint{UnixDomainSocketRemoteAgent::endpoint_for_testing},
            core::trust::remote::UnixDomainSocketAgent::proc_stat_start_time_resolver(),
            core::trust::remote::UnixDomainSocketAgent::Stub::get_sock_opt_credentials_resolver(),
            std::make_shared<core::trust::remote::UnixDomainSocketAgent::Stub::Session::Registry>()
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

    core::trust::remote::UnixDomainSocketAgent::ProcessStartTimeResolver to_functional()
    {
        return [this](pid_t pid)
        {
            return resolve_process_start_time(pid);
        };
    }

    MOCK_METHOD1(resolve_process_start_time, std::int64_t(pid_t));
};

}

TEST_F(UnixDomainSocketRemoteAgent, accepts_connections_on_endpoint)
{
    auto stub = core::trust::remote::UnixDomainSocketAgent::Stub::create_stub_for_configuration(
                the_default_stub_configuration());

    core::posix::ChildProcess child = core::posix::fork(
                UnixDomainSocketRemoteAgent::a_raw_peer_immediately_exiting(),
                core::posix::StandardStream::empty);

    auto result = child.wait_for(core::posix::wait::Flags::untraced);

    EXPECT_EQ(core::posix::wait::Result::Status::exited, result.status);
    EXPECT_EQ(core::posix::exit::Status::success, result.detail.if_exited.status);

    EXPECT_TRUE(stub->session_registry->has_session_for_uid(::getuid()));
}

TEST_F(UnixDomainSocketRemoteAgent, queries_peer_credentials_for_incoming_connection)
{
    static const uid_t uid{42};
    bool peer_credentials_queried{false};

    auto config = the_default_stub_configuration();

    config.peer_credentials_resolver = [&peer_credentials_queried](int)
    {
        peer_credentials_queried = true;
        return std::make_tuple(uid, 42, 42);
    };

    auto stub = core::trust::remote::UnixDomainSocketAgent::Stub::create_stub_for_configuration(config);

    core::posix::ChildProcess child = core::posix::fork(
                UnixDomainSocketRemoteAgent::a_raw_peer_immediately_exiting(),
                core::posix::StandardStream::empty);

    auto result = child.wait_for(core::posix::wait::Flags::untraced);

    EXPECT_EQ(core::posix::wait::Result::Status::exited, result.status);
    EXPECT_EQ(core::posix::exit::Status::success, result.detail.if_exited.status);

    EXPECT_TRUE(peer_credentials_queried);
    EXPECT_TRUE(stub->session_registry->has_session_for_uid(uid));
}

TEST_F(UnixDomainSocketRemoteAgent, stub_and_skeleton_query_process_start_time_for_request)
{
    using namespace ::testing;

    // skeleton --| good to go |--> stub
    core::testing::CrossProcessSync cps;

    NiceMock<MockProcessStartTimeResolver> process_start_time_resolver;

    auto config = the_default_stub_configuration();
    config.start_time_resolver = process_start_time_resolver.to_functional();

    auto stub = core::trust::remote::UnixDomainSocketAgent::Stub::create_stub_for_configuration(config);

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

        EXPECT_CALL(*mock_agent, prompt_user_for_request(_, _, _, _))
                .Times(1)
                .WillRepeatedly(Return(core::trust::Request::Answer::denied));

        core::trust::remote::UnixDomainSocketAgent::Skeleton::Configuration config
        {
            mock_agent,
            io_service,
            boost::asio::local::stream_protocol::endpoint{UnixDomainSocketRemoteAgent::endpoint_for_testing},
            process_start_time_resolver.to_functional(),
            core::trust::remote::UnixDomainSocketAgent::Skeleton::aa_get_task_con_app_id_resolver(),
            "Just a test for %1%."
        };

        core::trust::remote::UnixDomainSocketAgent::Skeleton skeleton{config};

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

    EXPECT_EQ(core::trust::Request::Answer::denied, stub->prompt_user_for_request(getuid(), 42, "", ""));

    child.send_signal_or_throw(core::posix::Signal::sig_term);
    auto result = child.wait_for(core::posix::wait::Flags::untraced);

    EXPECT_EQ(core::posix::wait::Result::Status::exited, result.status);
    EXPECT_EQ(core::posix::exit::Status::success, result.detail.if_exited.status);
}

