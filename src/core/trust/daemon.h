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

#ifndef CORE_TRUST_DAEMON_H_
#define CORE_TRUST_DAEMON_H_

#include <core/trust/agent.h>
#include <core/trust/store.h>

#include <core/trust/remote/agent.h>
#include <core/trust/remote/unix_domain_socket_agent.h>

#include <core/posix/exit.h>

#include <functional>
#include <map>
#include <set>

namespace core
{
namespace trust
{
// Encapsulates an executable that allows services to run an out-of-process daemon for
// managing trust to applications. The reason here is simple: Patching each and every
// service in the system to use and link against the trust-store might not be feasible.
// In those cases, an out-of-process store would be started given the services name. The
// store exposes itself to the bus and subscribes to one or more request gates, processing
// the incoming requests by leveraging an agent implementation.
struct Daemon
{
    // Just to safe some typing.
    typedef std::map<std::string, std::string> Dictionary;

    // Functor for creating trust::Agent instances.
    typedef std::function<
        std::shared_ptr<core::trust::Agent>(
            const std::string&, // The name of the service.
            const Dictionary&)  // Dictionary containing Agent-specific configuration options.
    > LocalAgentFactory;

    // Functor for creating trust::remote::Agent instances given a
    // set of parameters.
    typedef std::function<
        std::shared_ptr<core::trust::remote::Agent::Skeleton>(
            const std::string&,             // The name of the service.
            const std::shared_ptr<Store>&,  // The local store implementation.
            const std::shared_ptr<Agent>&,  // The local agent implementation.
            const Dictionary&               // Dictionary containing Agent-specific configuration options.
        )
    > RemoteAgentFactory;

    // Returns a map for resolving names to local agent factories.
    static const std::map<std::string, LocalAgentFactory>& known_local_agent_factories();

    // Returns a map for resolving names to remote agent factories.
    static const std::map<std::string, RemoteAgentFactory>&  known_remote_agent_factories();

    // Command-line parameters, their name and their description
    struct Parameters
    {
        Parameters() = delete;

        struct ForService
        {
            static constexpr const char* name{"for-service"};
            static constexpr const char* description{"The name of the service to handle trust for"};
        };

        struct LocalAgent
        {
            static constexpr const char* name{"local-agent"};
            static constexpr const char* description{"The local agent implementation"};
        };

        struct RemoteAgent
        {
            static constexpr const char* name{"remote-agent"};
            static constexpr const char* description{"The remote agent implementation"};
        };
    };

    // Collects all parameters for executing the daemon
    struct Configuration
    {
        // Parses the configuration from the given command line.
        static Configuration parse_from_command_line(int argc, char** argv);

        // The name of the service that the daemon should serve for.
        std::string service_name;

        // All local, i.e., actual implementations
        struct
        {
            // The store used for caching purposes.
            std::shared_ptr<Store> store;
            // The agent used for prompting the user.
            std::shared_ptr<Agent> agent;
        } local;

        // All remote implementations for exposing the services
        // of this daemon instance.
        struct
        {
            std::shared_ptr<core::trust::remote::Agent::Skeleton> agent;
        } remote;
    };

    // Executes the daemon with the given configuration.
    static core::posix::exit::Status main(const Configuration& configuration);
};
}
}

#endif // CORE_TRUST_DAEMON_H_


