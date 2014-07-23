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

#include <core/dbus/asio/executor.h>
#include <core/dbus/codec.h>
#include <core/dbus/object.h>
#include <core/dbus/service.h>
#include <core/dbus/skeleton.h>

namespace dbus = core::dbus;

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

    bus->install_executor(dbus::asio::make_executor(bus));

    return bus;
}

template<typename Key, typename Value>
class Store
{
public:
    bool insert(const Key& key, const Value& value)
    {
        std::lock_guard<std::mutex> lg(guard);
        bool result = false;
        std::tie(std::ignore, result) = map.insert(std::make_pair(key, value));
        return result;
    }

    void erase(const Key& key)
    {
        std::lock_guard<std::mutex> lg(guard);
        map.erase(key);
    }
private:
    std::mutex guard;
    std::map<Key, Value> map;
};

struct Token : public core::trust::Token
{
    Token(const std::string& service_name,
          const std::shared_ptr<dbus::Bus>& bus,
          const std::shared_ptr<core::trust::Store>& store)
        : store(store),
          bus(bus),
          service(dbus::Service::add_service(bus, service_name)),
          object(service->add_object_for_path(dbus::types::ObjectPath::root()))
    {
        object->install_method_handler<core::trust::dbus::Store::Add>([this](const core::dbus::Message::Ptr& msg)
        {
            handle_add(msg);
        });

        object->install_method_handler<core::trust::dbus::Store::Reset>([this](const core::dbus::Message::Ptr& msg)
        {
            handle_reset(msg);
        });

        object->install_method_handler<core::trust::dbus::Store::AddQuery>([this](const core::dbus::Message::Ptr& msg)
        {
            handle_add_query(msg);
        });

        object->install_method_handler<core::trust::dbus::Store::RemoveQuery>([this](const core::dbus::Message::Ptr& msg)
        {
            handle_remove_query(msg);
        });

        worker = std::move(std::thread([this](){Token::bus->run();}));
    }

    ~Token()
    {
        object->uninstall_method_handler<core::trust::dbus::Store::Add>();
        object->uninstall_method_handler<core::trust::dbus::Store::Reset>();
        object->uninstall_method_handler<core::trust::dbus::Store::AddQuery>();
        object->uninstall_method_handler<core::trust::dbus::Store::RemoveQuery>();

        bus->stop();

        if (worker.joinable())
            worker.join();
    }

    void handle_add(const core::dbus::Message::Ptr& msg)
    {
        core::trust::Request request;
        msg->reader() >> request;

        try
        {
            store->add(request);
        } catch(const std::runtime_error& e)
        {
            auto error = dbus::Message::make_error(
                        msg,
                        core::trust::dbus::Store::Error::AddingRequest::name(),
                        e.what());

            bus->send(error);
            return;
        }

        auto reply = dbus::Message::make_method_return(msg);
        bus->send(reply);
    }

    void handle_reset(const core::dbus::Message::Ptr& msg)
    {
        try
        {
            store->reset();
        } catch(const core::trust::Store::Errors::ErrorResettingStore& e)
        {
            auto error = core::dbus::Message::make_error(
                        msg,
                        core::trust::dbus::Store::Error::ResettingStore::name(),
                        e.what());

            bus->send(error);
        }

        auto reply = dbus::Message::make_method_return(msg);
        bus->send(reply);
    }

    void handle_add_query(const core::dbus::Message::Ptr& msg)
    {
        static std::uint64_t query_counter = 0;

        try
        {
            core::dbus::types::ObjectPath path{"/queries/" + std::to_string(query_counter++)};
            auto query = store->query();
            auto object = service->add_object_for_path(path);

            object->install_method_handler<core::trust::dbus::Store::Query::All>([this, query](const core::dbus::Message::Ptr& msg)
            {
                query->all();

                auto reply = core::dbus::Message::make_method_return(msg);
                bus->send(reply);
            });
            object->install_method_handler<core::trust::dbus::Store::Query::Current>([this, query](const core::dbus::Message::Ptr& msg)
            {
                try
                {
                    auto request = query->current();
                    auto reply = core::dbus::Message::make_method_return(msg);
                    reply->writer() << request;
                    bus->send(reply);
                } catch(const core::trust::Store::Query::Errors::NoCurrentResult& e)
                {
                    auto error = core::dbus::Message::make_error(
                                msg,
                                core::trust::dbus::Store::Query::Error::NoCurrentRequest::name(),
                                e.what());

                    bus->send(error);
                }
            });
            object->install_method_handler<core::trust::dbus::Store::Query::Erase>([this, query](const core::dbus::Message::Ptr& msg)
            {
                query->erase();

                auto reply = core::dbus::Message::make_method_return(msg);
                bus->send(reply);
            });
            object->install_method_handler<core::trust::dbus::Store::Query::Execute>([this, query](const core::dbus::Message::Ptr& msg)
            {
                query->execute();

                auto reply = core::dbus::Message::make_method_return(msg);
                bus->send(reply);
            });
            object->install_method_handler<core::trust::dbus::Store::Query::ForAnswer>([this, query](const core::dbus::Message::Ptr& msg)
            {
                core::trust::Request::Answer a; msg->reader() >> a;
                query->for_answer(a);

                auto reply = core::dbus::Message::make_method_return(msg);
                bus->send(reply);
            });
            object->install_method_handler<core::trust::dbus::Store::Query::ForApplicationId>([this, query](const core::dbus::Message::Ptr& msg)
            {
                std::string app_id; msg->reader() >> app_id;
                query->for_application_id(app_id);

                auto reply = core::dbus::Message::make_method_return(msg);
                bus->send(reply);
            });
            object->install_method_handler<core::trust::dbus::Store::Query::ForFeature>([this, query](const core::dbus::Message::Ptr& msg)
            {
                std::uint64_t feature; msg->reader() >> feature;
                query->for_feature(core::trust::Feature{feature});

                auto reply = core::dbus::Message::make_method_return(msg);
                bus->send(reply);
            });
            object->install_method_handler<core::trust::dbus::Store::Query::ForInterval>([this, query](const core::dbus::Message::Ptr& msg)
            {
                std::tuple<std::int64_t, std::int64_t> interval; msg->reader() >> interval;

                auto begin = core::trust::Request::Timestamp{core::trust::Request::Duration{std::get<0>(interval)}};
                auto end = core::trust::Request::Timestamp{core::trust::Request::Duration{std::get<1>(interval)}};

                query->for_interval(begin, end);

                auto reply = core::dbus::Message::make_method_return(msg);
                bus->send(reply);
            });
            object->install_method_handler<core::trust::dbus::Store::Query::Next>([this, query](const core::dbus::Message::Ptr& msg)
            {
                query->next();

                auto reply = core::dbus::Message::make_method_return(msg);
                bus->send(reply);
            });
            object->install_method_handler<core::trust::dbus::Store::Query::Status>([this, query](const core::dbus::Message::Ptr& msg)
            {
                auto reply = core::dbus::Message::make_method_return(msg);
                reply->writer() << query->status();
                bus->send(reply);
            });

            query_store.insert(path, object);

            auto reply = dbus::Message::make_method_return(msg);
            reply->writer() << path;
            bus->send(reply);
        } catch(const std::runtime_error& e)
        {
            auto error = core::dbus::Message::make_error(
                        msg,
                        core::trust::dbus::Store::Error::CreatingQuery::name(),
                        e.what());

            bus->send(error);
        }
    }

    void handle_remove_query(const core::dbus::Message::Ptr& msg)
    {
        try
        {
            core::dbus::types::ObjectPath path; msg->reader() >> path;
            query_store.erase(path);

            auto reply = dbus::Message::make_method_return(msg);
            bus->send(reply);
        } catch(...)
        {
        }

        auto reply = dbus::Message::make_method_return(msg);
        reply->writer();
        bus->send(reply);
    }

    std::shared_ptr<core::trust::Store> store;
    std::shared_ptr<dbus::Bus> bus;
    std::shared_ptr<dbus::Service> service;
    std::shared_ptr<dbus::Object> object;
    std::thread worker;

    detail::Store<core::dbus::types::ObjectPath, std::shared_ptr<core::dbus::Object>> query_store;
};
}
}

std::unique_ptr<core::trust::Token>
core::trust::expose_store_to_bus_with_name(
        const std::shared_ptr<core::trust::Store>& store,
        const std::shared_ptr<core::dbus::Bus>& bus,
        const std::string& name)
{
    if (name.empty())
        throw Errors::ServiceNameMustNotBeEmpty{};

    return std::move(std::unique_ptr<core::trust::Token>
    {
        new detail::Token
        {
            "com.ubuntu.trust.store." + name,
            bus,
            store
        }
    });
}

std::unique_ptr<core::trust::Token>
core::trust::expose_store_to_session_with_name(
        const std::shared_ptr<core::trust::Store>& store,
        const std::string& name)
{
    return std::move(core::trust::expose_store_to_bus_with_name(store, detail::session_bus(), name));
}
