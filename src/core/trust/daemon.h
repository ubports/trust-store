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

#include <core/posix/exit.h>

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
    // Abstracts listeners for incoming requests, possible implementations:
    //   * Listening to a socket
    //   * DBus
    struct RequestGate
    {
        typedef std::function<core::trust::Request::Answer(pid_t, const std::string&, const std::string&)> Handler;

        RequestGate() = default;
        virtual ~RequestGate() = default;


    };

    // Collects all parameters for executing the daemon
    struct Configuration
    {
        // The name of the service that the daemon should serve for.
        std::string service_name;
    };

    // Executes the daemon with the given configuration.
    static core::posix::exit::Status main(const Configuration& configuration);
};
}
}

#endif // CORE_TRUST_DAEMON_H_


