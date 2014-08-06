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

#ifndef CORE_TRUST_DBUSAGENT_H_
#define CORE_TRUST_DBUSAGENT_H_

#include <core/trust/visibility.h>

namespace core
{
namespace dbus
{
class Bus;
}
namespace trust
{
// Forward declare the Agent interface.
class Agent;

namespace dbus
{
/**
 * @brief create_per_user_agent_for_bus_connection creates a trust::Agent implementation communicating with a remote agent
 * implementation living in the same user session.
 * @param connection An existing DBus connection.
 * @param service_name The name of the service we are operating for.
 * @throws std::runtime_error in case of issues.
 */
CORE_TRUST_DLL_PUBLIC std::shared_ptr<core::trust::Agent> create_per_user_agent_for_bus_connection(
        const std::shared_ptr<core::dbus::Bus>& connection,
        const std::string& service_name);

/**
 * @brief create_multi_user_agent_for_bus_connection creates a trust::Agent implementation communicating with user-specific
 * remote agent implementations, living in user sessions.
 * @param connection An existing DBus connection.
 * @param service_name The name of the service we are operating for.
 * @throws std::runtime_error in case of issues.
 */
CORE_TRUST_DLL_PUBLIC std::shared_ptr<core::trust::Agent> create_multi_user_agent_for_bus_connection(
        const std::shared_ptr<core::dbus::Bus>& connection,
        const std::string& service_name);
}
}
}

#endif // CORE_TRUST_DBUSAGENT_H_
