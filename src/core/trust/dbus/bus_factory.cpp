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

#include <core/trust/dbus/bus_factory.h>
#include <core/trust/runtime.h>

#include <core/posix/this_process.h>

namespace env = core::posix::this_process::env;

namespace
{
class BusFactory : public core::trust::dbus::BusFactory
{
    core::dbus::Bus::Ptr bus_for_name(const std::string& bus_name) override
    {
        core::dbus::Bus::Ptr bus;

        if (bus_name == "system")
            bus = std::make_shared<core::dbus::Bus>(core::dbus::WellKnownBus::system);
        else if (bus_name == "session")
            bus = std::make_shared<core::dbus::Bus>(core::dbus::WellKnownBus::session);
        else if (bus_name == "system_with_address_from_env")
            bus = std::make_shared<core::dbus::Bus>(env::get_or_throw("DBUS_SYSTEM_BUS_ADDRESS"));
        else if (bus_name == "session_with_address_from_env")
            bus = std::make_shared<core::dbus::Bus>(env::get_or_throw("DBUS_SESSION_BUS_ADDRESS"));

        if (not bus) throw std::runtime_error
        {
            "Could not create bus for name: " + bus_name
        };

        bus->install_executor(core::dbus::asio::make_executor(bus, core::trust::Runtime::instance().io_service));
        return bus;
    }
};
}


