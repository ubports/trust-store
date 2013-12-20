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

#include <core/trust/expose.h>

#include <core/trust/store.h>

#include "codec.h"
#include "dbus_interface.h"

#include <org/freedesktop/dbus/asio/executor.h>
#include <org/freedesktop/dbus/codec.h>
#include <org/freedesktop/dbus/service.h>
#include <org/freedesktop/dbus/skeleton.h>

namespace dbus = org::freedesktop::dbus;

namespace
{
namespace detail
{
std::shared_ptr<dbus::Bus> session_bus()
{
    std::shared_ptr<dbus::Bus> bus
    {
        new dbus::Bus(dbus::WellKnownBus::session)
    };

    std::shared_ptr<dbus::Executor> executor(new dbus::asio::Executor{bus});
    bus->install_executor(executor);

    return bus;
}

struct Token : public core::trust::Token, public dbus::Skeleton<core::trust::DBusInterface>
{
    Token(const std::shared_ptr<dbus::Bus>& bus,
          const std::shared_ptr<core::trust::Store>& store)
        : dbus::Skeleton<core::trust::DBusInterface>(bus),
          store(store),
          object(access_service()->add_object_for_path(dbus::types::ObjectPath::root()))
    {
        object->install_method_handler<core::trust::DBusInterface::Add>([this](DBusMessage* msg)
        {
            handle_add(msg);
        });

        object->install_method_handler<core::trust::DBusInterface::Reset>([this](DBusMessage* msg)
        {
            handle_reset(msg);
        });

        worker = std::move(std::thread([this](){access_bus()->run();}));
    }

    ~Token()
    {
        object->uninstall_method_handler<core::trust::DBusInterface::Add>();
        object->uninstall_method_handler<core::trust::DBusInterface::Reset>();

        access_bus()->stop();
        if (worker.joinable())
            worker.join();
    }

    void handle_add(DBusMessage* message)
    {
        core::trust::Request request;
        auto msg = dbus::Message::from_raw_message(message);
        msg->reader() >> request;

        try
        {
            store->add(request);
        } catch(const std::runtime_error& e)
        {
            auto error = dbus::Message::make_error(
                        message,
                        core::trust::DBusInterface::Error::AddingRequest::name(),
                        e.what());

            access_bus()->send(error->get());
            return;
        }

        auto reply = dbus::Message::make_method_return(message);
        access_bus()->send(reply->get());
    }

    void handle_reset(DBusMessage* msg)
    {
        try
        {
            store->reset();
        } catch(const core::trust::Store::Errors::ErrorResettingStore& e)
        {
            auto error = dbus::Message::make_error(
                        msg,
                        core::trust::DBusInterface::Error::ResettingStore::name(),
                        e.what());

            access_bus()->send(error->get());
        }

        auto reply = dbus::Message::make_method_return(msg);
        access_bus()->send(reply->get());
    }

    std::shared_ptr<core::trust::Store> store;
    std::shared_ptr<dbus::Object> object;
    std::thread worker;
};
}
}

std::unique_ptr<core::trust::Token>
core::trust::expose_store_to_session_with_name(
        const std::shared_ptr<core::trust::Store>& store,
        const std::string& name)
{
    if (name.empty())
        throw Error::ServiceNameMustNotBeEmpty{};

    core::trust::DBusInterface::mutable_name() = "com.ubuntu.trust.store." + name;
    return std::move(std::unique_ptr<core::trust::Token>(new detail::Token{detail::session_bus(), store}));
}
