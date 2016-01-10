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

#include <core/dbus/bus.h>

namespace core
{
namespace trust
{
namespace dbus
{
class BusFactory
{
public:
    typedef std::shared_ptr<BusFactory> Ptr;

    static Ptr create_default();

    BusFactory(const BusFactory&) = delete;
    BusFactory(BusFactory&&) = delete;
    virtual ~BusFactory() = default;

    BusFactory& operator=(const BusFactory&) = delete;
    BusFactory& operator=(BusFactory&&) = delete;

    core::dbus::Bus::Ptr bus_for_name(const std::string& name) = 0;
protected:
    BusFactory() = default;
};
}
}
}

#endif // CORE_TRUST_DBUS_BUS_FACTORY_H_

