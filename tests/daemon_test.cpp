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

#include <core/trust/daemon.h>

#include "process_exited_successfully.h"

#include <core/posix/exec.h>
#include <core/posix/fork.h>

#include <gmock/gmock.h>

#include <thread>

namespace
{
static constexpr const char* service_name
{
    "UnlikelyToEverExistOutsideOfTesting"
};

static constexpr const char* endpoint
{
    "/tmp/unlikely.to.ever.exist.outside.of.testing"
};

}

TEST(Daemon, unix_domain_agents_for_stub_and_skeleton_work_as_expected)
{
    std::remove(endpoint);

    // The stub accepting trust requests, relaying them via
    // the configured remote agent.
    core::posix::ChildProcess stub = core::posix::fork([]()
    {
        const char* argv[] =
        {
            __PRETTY_FUNCTION__,
            "--for-service", service_name,
            "--remote-agent", "UnixDomainSocketRemoteAgent",
            "--endpoint=/tmp/unlikely.to.ever.exist.outside.of.testing"
        };

        auto configuration = core::trust::Daemon::Stub::Configuration::from_command_line(6, argv);

        return core::trust::Daemon::Stub::main(configuration);
    }, core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);

    // We really want to write EXPECT_TRUE(WaitForEndPointToBecomeAvailableFor(endpoint, 500ms));
    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    // The skeleton announcing itself to the stub instance started before via
    // the endpoint specified for the remote agent. In addition, it features a local
    // agent instance that always replies: denied.
    auto skeleton = core::posix::fork([]()
    {
        const char* argv[]
        {
            __PRETTY_FUNCTION__,
            "--remote-agent", "UnixDomainSocketRemoteAgent",
            "--endpoint=/tmp/unlikely.to.ever.exist.outside.of.testing",
            "--local-agent", "TheAlwaysDenyingLocalAgent",
            "--for-service", service_name
        };

        auto configuration = core::trust::Daemon::Skeleton::Configuration::from_command_line(8, argv);

        return core::trust::Daemon::Skeleton::main(configuration);
    }, core::posix::StandardStream::empty);

    // Wait for everything to be setup
    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    // And inject a request into the stub
    std::string answer;

    for (int feature = 0; feature < 50; feature++)
    {
        stub.cin() << core::posix::this_process::instance().pid() << " " << ::getuid() << " " << feature << std::endl;
        stub.cout() >> answer;
    }

    // Wait for all requests to be answered
    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    // Sigterm both stub and skeleton.
    skeleton.send_signal_or_throw(core::posix::Signal::sig_term);
    stub.send_signal_or_throw(core::posix::Signal::sig_term);

    // Expect both of them to exit with success.
    EXPECT_TRUE(ProcessExitedSuccessfully(skeleton.wait_for(core::posix::wait::Flags::untraced)));
    EXPECT_TRUE(ProcessExitedSuccessfully(stub.wait_for(core::posix::wait::Flags::untraced)));
}
