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

#include <core/dbus/fixture.h>
#include <core/dbus/asio/executor.h>

#include <core/trust/expose.h>
#include <core/trust/resolve.h>

#include <core/trust/store.h>

#include <core/posix/signal.h>
#include <core/testing/cross_process_sync.h>
#include <core/testing/fork_and_run.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace
{
static const std::string service_name{"does_not_exist"};

struct RemoteTrustStore : public core::dbus::testing::Fixture
{
};

auto session_bus_config_file =
        core::dbus::testing::Fixture::default_session_bus_config_file() =
        core::testing::session_bus_configuration_file();

auto system_bus_config_file =
        core::dbus::testing::Fixture::default_system_bus_config_file() =
        core::testing::system_bus_configuration_file();
}

TEST_F(RemoteTrustStore, a_store_exposed_to_the_session_can_be_reset)
{
    core::testing::CrossProcessSync cps;

    auto service = [this, &cps]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return core::posix::exit::Status::success;
    };

    auto client = [this, &cps]()
    {
        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);
        std::cout << "here" << std::endl;
        EXPECT_NO_THROW(store->reset(););

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
            core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST_F(RemoteTrustStore, a_store_exposed_to_the_session_can_be_added_to)
{
    core::testing::CrossProcessSync cps1, cps2;
    static const unsigned int request_count = 100;

    core::trust::Request prototype_request
    {
        "com.does.not.exist.app1",
        0,
        std::chrono::system_clock::time_point{std::chrono::seconds(500)},
        core::trust::Request::Answer::granted
    };

    auto service = [this, prototype_request, &cps1]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
           trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name); store->reset();
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps1.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
            core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [this, prototype_request, &cps1]()
    {
        core::trust::Request r
        {
            prototype_request.from,
            prototype_request.feature,
            prototype_request.when,
            prototype_request.answer
        };

        cps1.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);

        for (unsigned int i = 0; i < request_count; i++)
        {
            r.feature = i;
            store->add(r);
        }

        // Resetting the feature counter and checking if all requests have been stored.
        r.feature = 0;
        auto query = store->query();
        query->execute();
        EXPECT_EQ(core::trust::Store::Query::Status::has_more_results, query->status());
        while(query->status() != core::trust::Store::Query::Status::eor)
        {
            EXPECT_EQ(r, query->current());
            query->next();
            r.feature++;
        }

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
            core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

// Taken from trust store test
namespace
{
core::testing::CrossProcessSync cps;
}

TEST_F(RemoteTrustStore, limiting_query_to_app_id_returns_correct_results)
{
    auto service = [this]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
           trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return core::posix::exit::Status::success;
    };

    auto client = [this]()
    {
        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);
        store->reset();

        const std::string app1{"com.does.not.exist.app1"};
        const std::string app2{"com.does.not.exist.app2"};

        core::trust::Request r1
        {
            app1,
            0,
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::granted
        };

        core::trust::Request r2
        {
            app2,
            1,
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::granted
        };

        store->add(r1);
        store->add(r2);

        auto query = store->query();
        query->for_application_id(app2);
        query->execute();

        EXPECT_EQ(core::trust::Store::Query::Status::has_more_results, query->status());
        EXPECT_EQ(r2, query->current());

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST_F(RemoteTrustStore, limiting_query_to_feature_returns_correct_results)
{
    auto service = [this]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
           trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return core::posix::exit::Status::success;
    };

    auto client = [this]()
    {
        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);
        store->reset();

        const std::string app1{"com.does.not.exist.app1"};

        core::trust::Request r1
        {
            app1,
            0,
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::granted
        };

        core::trust::Request r2
        {
            app1,
            1,
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::granted
        };

        store->add(r1);
        store->add(r2);

        auto query = store->query();
        query->for_feature(r2.feature);
        query->execute();

        EXPECT_EQ(core::trust::Store::Query::Status::has_more_results, query->status());
        EXPECT_EQ(r2, query->current());

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST_F(RemoteTrustStore, limiting_query_to_answer_returns_correct_results)
{
    auto service = [this]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
           trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return core::posix::exit::Status::success;
    };

    auto client = [this]()
    {
        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);
        store->reset();

        const std::string app1{"com.does.not.exist.app1"};

        core::trust::Request r1
        {
            app1,
            0,
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::granted
        };

        core::trust::Request r2
        {
            app1,
            1,
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::denied
        };

        store->add(r1);
        store->add(r2);

        auto query = store->query();
        query->for_answer(r2.answer);
        query->execute();

        EXPECT_EQ(core::trust::Store::Query::Status::has_more_results, query->status());
        EXPECT_EQ(r2, query->current());

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST_F(RemoteTrustStore, limiting_query_to_time_interval_returns_correct_result)
{
    auto service = [this]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return core::posix::exit::Status::success;
    };

    auto client = [this]()
    {
        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);
        store->reset();

        const std::string app1{"com.does.not.exist.app1"};

        core::trust::Request r1
        {
            app1,
            0,
            std::chrono::system_clock::time_point(std::chrono::seconds{0}),
            core::trust::Request::Answer::granted
        };

        core::trust::Request r2
        {
            app1,
            1,
            std::chrono::system_clock::time_point(std::chrono::seconds{500}),
            core::trust::Request::Answer::granted
        };

        core::trust::Request r3
        {
            app1,
            1,
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::granted
        };

        store->add(r1);
        store->add(r2);
        store->add(r3);

        auto query = store->query();
        query->for_interval(
                    std::chrono::system_clock::time_point(std::chrono::seconds{500}),
                    std::chrono::system_clock::now());
        query->execute();

        EXPECT_EQ(core::trust::Store::Query::Status::has_more_results, query->status());
        EXPECT_EQ(r2, query->current()); query->next();
        EXPECT_EQ(r3, query->current()); query->next();
        EXPECT_EQ(core::trust::Store::Query::Status::eor, query->status());

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST_F(RemoteTrustStore, limiting_query_to_time_interval_and_answer_returns_correct_result)
{
    auto service = [this]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return core::posix::exit::Status::success;
    };

    auto client = [this]()
    {
        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);
        store->reset();

        const std::string app1{"com.does.not.exist.app1"};

        core::trust::Request r1
        {
            app1,
            0,
            std::chrono::system_clock::time_point(std::chrono::seconds{0}),
            core::trust::Request::Answer::granted
        };

        core::trust::Request r2
        {
            app1,
            1,
            std::chrono::system_clock::time_point(std::chrono::seconds{500}),
            core::trust::Request::Answer::granted
        };

        core::trust::Request r3
        {
            app1,
            1,
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::denied
        };

        store->add(r1);
        store->add(r2);
        store->add(r3);

        auto query = store->query();
        query->for_interval(
                    std::chrono::system_clock::time_point(std::chrono::seconds{500}),
                    std::chrono::system_clock::now());
        query->for_answer(core::trust::Request::Answer::denied);
        query->execute();

        EXPECT_EQ(core::trust::Store::Query::Status::has_more_results, query->status());
        EXPECT_EQ(r3, query->current()); query->next();
        EXPECT_EQ(core::trust::Store::Query::Status::eor, query->status());

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST_F(RemoteTrustStore, added_requests_are_found_by_query_multi_threaded)
{
    auto service = [this]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return core::posix::exit::Status::success;
    };

    auto client = [this]()
    {
        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});
        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);

        store->reset();

        static const std::uint64_t base_feature = 0;

        auto inserter = [store](int base)
        {
            core::trust::Request r
            {
                "this.does.not.exist.app",
                base_feature,
                std::chrono::system_clock::now(),
                core::trust::Request::Answer::granted
            };

            for (unsigned int i = 0; i < 100; i++)
            {
                r.feature = base + i;
                try
                {
                    store->add(r);
                } catch(...)
                {

                }
            }
        };

        std::thread t1 {inserter, 0};
        std::thread t2 {inserter, 100};
        std::thread t3 {inserter, 200};
        std::thread t4 {inserter, 300};
        std::thread t5 {inserter, 400};

        t1.join(); t2.join(); t3.join(); t4.join(); t5.join();

        auto query = store->query();
        query->all();
        query->execute();

        unsigned int counter = 0;
        while(core::trust::Store::Query::Status::eor != query->status())
        {
            query->next();
            counter++;
        }

        EXPECT_EQ(500u, counter);

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST_F(RemoteTrustStore, erasing_requests_empties_store)
{
    auto service = [this]()
    {
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        trap->signal_raised().connect([trap](core::posix::Signal)
        {
            trap->stop();
        });

        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_bus_with_name(store, bus, service_name);

        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        trap->run();

        return core::posix::exit::Status::success;
    };

    auto client = [this]()
    {
        auto bus = session_bus();
        bus->install_executor(core::dbus::asio::make_executor(bus));

        cps.wait_for_signal_ready_for(std::chrono::milliseconds{500});
        auto store = core::trust::resolve_store_on_bus_with_name(bus, service_name);
        store->reset();

        // Insert a bunch of requests and erase them after that.
        {
            core::trust::Request r
            {
                "this.does.not.exist.app",
                0,
                std::chrono::system_clock::now(),
                core::trust::Request::Answer::granted
            };

            for (unsigned int i = 0; i < 100; i++)
            {
                r.feature = i;
                store->add(r);
            }

            auto query = store->query();
            query->execute();

            while(core::trust::Store::Query::Status::eor != query->status())
            {
                query->erase();
            }
        }

        // Now let's see if the records are actually gone.
        auto query = store->query();
        query->execute();
        EXPECT_EQ(core::trust::Store::Query::Status::eor, query->status());

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
                    core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}
