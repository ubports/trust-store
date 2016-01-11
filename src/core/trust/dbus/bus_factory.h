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

#ifndef CORE_TRUST_DBUS_BUS_FACTORY_H_
#define CORE_TRUST_DBUS_BUS_FACTORY_H_

#include <core/trust/visibility.h>

#include <core/dbus/bus.h>

#include <iosfwd>

namespace core
{
namespace trust
{
namespace dbus
{
// BusFactory abstracts creation of bus instances.
class CORE_TRUST_DLL_PUBLIC BusFactory
{
public:
    typedef std::shared_ptr<BusFactory> Ptr;

    // Type enumerates all different types of buses
    // that we can create instances for.
    enum class Type
    {
        session, // The default session bus.
        system,  // The default system bus.
        session_with_address_from_env, // Session bus with address as available in the process's env.
        system_with_address_from_env // System bus with address as available in the process's env.
    };

    // create_default returns an instance of the default implementation.
    static Ptr create_default();

    // @cond
    BusFactory(const BusFactory&) = delete;
    BusFactory(BusFactory&&) = delete;
    virtual ~BusFactory() = default;

    BusFactory& operator=(const BusFactory&) = delete;
    BusFactory& operator=(BusFactory&&) = delete;
    // @endcond

    // bus_for_type returns a dbus::Bus instance for type.
    virtual core::dbus::Bus::Ptr bus_for_type(Type type) = 0;
protected:
    BusFactory() = default;
};

// operator<< inserts the given BusFactory::Type instance into the given std::ostream.
CORE_TRUST_DLL_PUBLIC std::ostream& operator<<(std::ostream&, BusFactory::Type);
// operator>> extracts a BusFactory::Type instance from the given std::istream.
CORE_TRUST_DLL_PUBLIC std::istream& operator>>(std::istream&, BusFactory::Type&);
}
}
}

#endif // CORE_TRUST_DBUS_BUS_FACTORY_H_

