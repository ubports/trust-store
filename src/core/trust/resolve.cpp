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

#include <core/trust/resolve.h>

#include <core/trust/request.h>
#include <core/trust/store.h>

#include "codec.h"
#include "dbus_interface.h"

#include <org/freedesktop/dbus/service.h>
#include <org/freedesktop/dbus/stub.h>

namespace dbus = org::freedesktop::dbus;

namespace
{
namespace detail
{
std::shared_ptr<dbus::Bus> session_bus()
{
    return std::shared_ptr<dbus::Bus>
    {
        new dbus::Bus(dbus::WellKnownBus::session)
    };
}

struct Store :
        public core::trust::Store,
        public dbus::Stub<core::trust::DBusInterface>
{
    Store()
        : dbus::Stub<core::trust::DBusInterface>(session_bus()),
          proxy(access_service()->object_for_path(dbus::types::ObjectPath::root()))
    {
    }

    void add(const core::trust::Request& r)
    {
        auto response =
                proxy->invoke_method_synchronously<
                    core::trust::DBusInterface::Add,
                    void>(r);

        if (response.is_error())
            throw std::runtime_error(response.error());
    }

    void reset()
    {
        try
        {
            auto response =
                    proxy->invoke_method_synchronously<
                        core::trust::DBusInterface::Reset,
                        void>();

            if (response.is_error())
            {
                throw std::runtime_error(response.error());
            }
        } catch(const std::runtime_error& e)
        {
            std::cout << e.what() << std::endl;
        }


    }

    std::shared_ptr<core::trust::Store::Query> query()
    {
        return std::shared_ptr<core::trust::Store::Query>{};
    }

    std::shared_ptr<dbus::Object> proxy;
};
}
}

std::shared_ptr<core::trust::Store> core::trust::resolve_store_in_session_with_name(
        const std::string& name)
{
    if (name.empty())
        throw Error::ServiceNameMustNotBeEmpty{};

    core::trust::DBusInterface::mutable_name() = "com.ubuntu.trust.store." + name;
    return std::shared_ptr<core::trust::Store>{new detail::Store()};
}
