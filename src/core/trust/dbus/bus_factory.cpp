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

#include <core/dbus/asio/executor.h>
#include <core/posix/this_process.h>

#include <boost/lexical_cast.hpp>

namespace env = core::posix::this_process::env;

namespace
{
class DefaultBusFactory : public core::trust::dbus::BusFactory
{
    core::dbus::Bus::Ptr bus_for_type(Type type) override
    {
        core::dbus::Bus::Ptr bus;

        switch (type)
        {
        case Type::system:
            bus = std::make_shared<core::dbus::Bus>(core::dbus::WellKnownBus::system);
            break;
        case Type::session:
            bus = std::make_shared<core::dbus::Bus>(core::dbus::WellKnownBus::session);
            break;
        case Type::system_with_address_from_env:
            bus = std::make_shared<core::dbus::Bus>(env::get_or_throw("DBUS_SYSTEM_BUS_ADDRESS"));
            break;
        case Type::session_with_address_from_env:
            bus = std::make_shared<core::dbus::Bus>(env::get_or_throw("DBUS_SESSION_BUS_ADDRESS"));
            break;
        }

        if (not bus) throw std::runtime_error
        {
            "Could not create bus for name: " + boost::lexical_cast<std::string>(type)
        };

        bus->install_executor(core::dbus::asio::make_executor(bus, core::trust::Runtime::instance().service()));
        return bus;
    }
};
}

core::trust::dbus::BusFactory::Ptr core::trust::dbus::BusFactory::create_default()
{
    return std::make_shared<DefaultBusFactory>();
}

std::ostream& core::trust::dbus::operator<<(std::ostream& out, core::trust::dbus::BusFactory::Type type)
{
    switch (type)
    {
    case core::trust::dbus::BusFactory::Type::system:
        out << "system";
        break;
    case core::trust::dbus::BusFactory::Type::session:
        out << "session";
        break;
    case core::trust::dbus::BusFactory::Type::system_with_address_from_env:
        out << "system_with_address_from_env";
        break;
    case core::trust::dbus::BusFactory::Type::session_with_address_from_env:
        out << "session_with_address_from_env";
        break;
    }

    return out;
}

std::istream& core::trust::dbus::operator>>(std::istream& in, core::trust::dbus::BusFactory::Type& type)
{
    std::string s; in >> s;

    if (s == "system")
        type = core::trust::dbus::BusFactory::Type::system;
    else if (s == "session")
        type = core::trust::dbus::BusFactory::Type::session;
    else if (s == "system_with_address_from_env")
        type = core::trust::dbus::BusFactory::Type::system_with_address_from_env;
    else if (s == "session_with_address_from_env")
        type = core::trust::dbus::BusFactory::Type::session_with_address_from_env;

    return in;
}
