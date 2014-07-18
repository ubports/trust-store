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

#include <core/dbus/service.h>
#include <core/dbus/stub.h>

namespace dbus = core::dbus;

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

struct Store : public core::trust::Store
{
    Store(const std::shared_ptr<dbus::Service>& service,
          const std::shared_ptr<core::dbus::Bus>& bus)
        : bus(bus),
          worker{[this]() { Store::bus->run(); }},
          service(service),
          proxy(service->object_for_path(dbus::types::ObjectPath::root()))
    {
    }

    ~Store()
    {
        bus->stop();

        if (worker.joinable())
            worker.join();
    }

    struct Query : public core::trust::Store::Query
    {
        core::dbus::types::ObjectPath path;
        std::shared_ptr<dbus::Object> parent;
        std::shared_ptr<dbus::Object> object;

        Query(const core::dbus::types::ObjectPath& path,
              const std::shared_ptr<dbus::Object>& parent,
              const std::shared_ptr<dbus::Object>& object)
            : path(path),
              parent(parent),
              object(object)
        {
        }

        ~Query()
        {
            try
            {
                parent->invoke_method_synchronously<core::trust::dbus::Store::RemoveQuery, void>(path);
            } catch(...)
            {
            }
        }

        void all()
        {
            object->invoke_method_synchronously<core::trust::dbus::Store::Query::All, void>();
        }

        core::trust::Request current()
        {
            auto result = object->invoke_method_synchronously<
                    core::trust::dbus::Store::Query::Current,
                    core::trust::Request>();

            if (result.is_error())
            {
                throw core::trust::Store::Query::Errors::NoCurrentResult{};
            }

            return result.value();
        }

        void erase()
        {
            auto result = object->invoke_method_synchronously<core::trust::dbus::Store::Query::Erase, void>();

            if (result.is_error())
                throw std::runtime_error(result.error().print());
        }

        void execute()
        {
            auto result = object->invoke_method_synchronously<core::trust::dbus::Store::Query::Execute, void>();

            if (result.is_error())
                throw std::runtime_error(result.error().print());
        }

        void for_answer(core::trust::Request::Answer answer)
        {
            auto result = object->invoke_method_synchronously<core::trust::dbus::Store::Query::ForAnswer, void>(answer);

            if (result.is_error())
                throw std::runtime_error(result.error().print());
        }

        void for_application_id(const std::string &id)
        {
            auto result = object->invoke_method_synchronously<core::trust::dbus::Store::Query::ForApplicationId, void>(id);

            if (result.is_error())
                throw std::runtime_error(result.error().print());
        }

        void for_feature(std::uint64_t feature)
        {
            auto result = object->invoke_method_synchronously<core::trust::dbus::Store::Query::ForFeature, void>(feature);

            if (result.is_error())
                throw std::runtime_error(result.error().print());
        }

        void for_interval(const core::trust::Request::Timestamp &begin, const core::trust::Request::Timestamp &end)
        {
            auto result = object->invoke_method_synchronously<
                    core::trust::dbus::Store::Query::ForInterval,
                    void>(
                        std::make_tuple(
                            begin.time_since_epoch().count(),
                            end.time_since_epoch().count()));

            if (result.is_error())
                throw std::runtime_error(result.error().print());
        }

        void next()
        {
            auto result = object->invoke_method_synchronously<core::trust::dbus::Store::Query::Next, void>();

            if (result.is_error())
                throw std::runtime_error(result.error().print());
        }

        core::trust::Store::Query::Status status() const
        {
            auto result = object->invoke_method_synchronously<
                    core::trust::dbus::Store::Query::Status,
                    core::trust::Store::Query::Status>();

            if (result.is_error())
                throw std::runtime_error(result.error().print());

            return result.value();
        }
    };

    void add(const core::trust::Request& r)
    {
        auto response =
                proxy->invoke_method_synchronously<
                    core::trust::dbus::Store::Add,
                    void>(r);

        if (response.is_error())
            throw std::runtime_error(response.error().print());
    }

    void reset()
    {
        try
        {
            auto response =
                    proxy->invoke_method_synchronously<
                        core::trust::dbus::Store::Reset,
                        void>();

            if (response.is_error())
            {
                throw std::runtime_error(response.error().print());
            }
        } catch(const std::runtime_error& e)
        {
            std::cout << e.what() << std::endl;
        }
    }

    std::shared_ptr<core::trust::Store::Query> query()
    {
        auto result = proxy->invoke_method_synchronously<
                core::trust::dbus::Store::AddQuery,
                core::dbus::types::ObjectPath>();

        if (result.is_error())
            throw std::runtime_error(result.error().print());

        auto path = result.value();

        auto query = std::shared_ptr<core::trust::Store::Query>(new detail::Store::Query
        {
            path,
            proxy,
            service->object_for_path(path)
        });

        return query;
    }

    std::shared_ptr<core::dbus::Bus> bus;
    std::thread worker;
    std::shared_ptr<dbus::Service> service;
    std::shared_ptr<dbus::Object> proxy;
};
}
}

std::shared_ptr<core::trust::Store> core::trust::resolve_store_on_bus_with_name(
        const std::shared_ptr<core::dbus::Bus>& bus,
        const std::string& name)
{
    if (name.empty())
        throw Errors::ServiceNameMustNotBeEmpty{};

    return std::shared_ptr<core::trust::Store>
    {
        new detail::Store
        {
            core::dbus::Service::use_service(bus, "com.ubuntu.trust.store." + name),
            bus
        }
    };
}

std::shared_ptr<core::trust::Store> core::trust::resolve_store_in_session_with_name(
        const std::string& name)
{
    return core::trust::resolve_store_on_bus_with_name(detail::session_bus(), name);
}
