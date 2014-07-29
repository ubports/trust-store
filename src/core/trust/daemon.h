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

    // Collects all known local agents
    struct LocalAgents
    {
        LocalAgents() = delete;

        // Implements the trust::Agent interface and dispatches calls to a helper
        // prompt provider, tying it together with the requesting service and app
        // by leveraging Mir's trusted session/prompting support.
        struct MirAgent { static constexpr const char* name { "MirAgent" }; };

        // Implements the trust::Agent interface and dispatches calls to the user
        // leveraging whiptail to provide a dialog in a terminal.
        struct TerminalAgent { static constexpr const char* name { "TerminalAgent" }; };
    };

    // Collects all known remote agents
    struct RemoteAgents
    {
        RemoteAgents() = delete;

        // A remote agent implementation that exposes a unix domain socket in the
        // filesystem. The stub implementation exposes the socket and handles incoming
        // connections from its skeletons. For incoming requests, the stub selects the
        // handling skeleton based on the user id associated with the request.
        struct UnixDomainSocketRemoteAgent { static constexpr const char* name { "UnixDomainSocketRemoteAgent" }; };

        // A remote agent implementation leveraging dbus.
        struct DBusRemoteAgent { static constexpr const char* name { "DBusRemoteAgent" }; };
    };

    struct Skeleton
    {
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
            static Configuration from_command_line(int argc, const char** argv);

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

    struct Stub
    {
        // Functor for creating trust::Agent instances.
        typedef std::function<
            std::shared_ptr<core::trust::remote::Agent::Stub>(
                const std::string&, // The name of the service.
                const Dictionary&)  // Dictionary containing Agent-specific configuration options.
        > RemoteAgentFactory;

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
            static Configuration from_command_line(int argc, const char** argv);

            // The name of the service that the daemon should serve for.
            std::string service_name;

            // All remote agents.
            struct
            {
                // Trust requests are issued via this stub.
                std::shared_ptr<core::trust::remote::Agent::Stub> agent;
            } remote;
        };

        // Executes the daemon with the given configuration.
        static core::posix::exit::Status main(const Configuration& configuration);
    };
};
}
}

#endif // CORE_TRUST_DAEMON_H_


