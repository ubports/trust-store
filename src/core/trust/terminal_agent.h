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

#ifndef CORE_TRUST_TERMINAL_AGENT_H_
#define CORE_TRUST_TERMINAL_AGENT_H_

#include <core/trust/agent.h>

#include <core/posix/exec.h>

#include <iterator>

namespace core
{
namespace trust
{
// Agent implementation leveraging whiptail to display
// a dialog box in the terminal.
struct TerminalAgent : public core::trust::Agent
{
    // Default width of the dialog box
    static constexpr int default_width{70};
    // Default height of the dialog box
    static constexpr int default_height{10};

    // Path to the whiptail executable.
    static constexpr const char* whiptail
    {
        "/bin/whiptail"
    };

    // Constructs a new instance for the given service name.
    TerminalAgent(const std::string& service_name) : service_name{service_name}
    {
    }

    // From core::trust::Agent.
    Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters) override
    {
        std::map<std::string, std::string> env;

        core::posix::this_process::env::for_each([&env](const std::string& k, const std::string& v)
        {
            env.insert(std::make_pair(k, v));
        });

        std::vector<std::string> args
        {
            "--title", "\"Please audit access to: " + service_name + " by " + parameters.application.id + "\"",
            "--yes-button", "Grant",
            "--no-button", "Deny",
            "--yesno",
            "\"" + parameters.description + "\"", std::to_string(default_height), std::to_string(default_width)
        };

        std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, " "));

        auto whiptail_process = core::posix::exec(
                    whiptail,
                    args,
                    env,
                    core::posix::StandardStream::empty);

        auto result = whiptail_process.wait_for(core::posix::wait::Flags::untraced);

        // We now analyze the result of the process execution.
        if (core::posix::wait::Result::Status::exited != result.status) throw std::logic_error
        {
            "Unable to determine a conclusive answer from the user"
        };

        // If the child process did not exit cleanly, we deny access to the resource.
        if (core::posix::exit::Status::failure == result.detail.if_exited.status)
            return core::trust::Request::Answer::denied;

        return core::trust::Request::Answer::granted;
    }

    // The name of the service we are acting for.
    std::string service_name;
};
}
}

#endif // CORE_TRUST_TERMINAL_AGENT_H_

