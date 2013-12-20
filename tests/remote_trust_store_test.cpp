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
#include <core/trust/resolve.h>

#include <core/trust/store.h>

#include <core/testing/fork_and_run.h>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace
{
static const std::string service_name{"does_not_exist"};
}

TEST(RemoteTrustStore, a_store_exposed_to_the_session_can_be_reset)
{
    auto service = []()
    {
        static bool finish = false;
        ::signal(SIGTERM, [](int) { finish = true; });

        auto store = core::trust::create_default_store(service_name);
        auto mapping = core::trust::expose_store_to_session_with_name(store, service_name);

        while (!finish)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{500});
        }

        return core::posix::exit::Status::success;
    };

    auto client = []()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{500});

        auto store = core::trust::resolve_store_in_session_with_name(service_name);
        EXPECT_NO_THROW(store->reset(););

        return ::testing::Test::HasFatalFailure() || ::testing::Test::HasFailure() ?
            core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST(RemoteTrustStore, a_store_exposed_to_the_session_can_be_added_to)
{
    static const unsigned int request_count = 100;

    core::trust::Request prototype_request
    {
        "com.does.not.exist.app1",
        0,
        std::chrono::system_clock::time_point{std::chrono::seconds(500)},
        core::trust::Request::Answer::granted
    };

    auto service = [prototype_request]()
    {
        core::trust::Request r
        {
            prototype_request.from,
            prototype_request.feature,
            prototype_request.when,
            prototype_request.answer
        };

        static bool finish = false;
        ::signal(SIGTERM, [](int) { finish = true; });

        auto store = core::trust::create_default_store(service_name);
        store->reset();
        auto mapping = core::trust::expose_store_to_session_with_name(store, service_name);

        while (!finish)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{500});
        }

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

    auto client = [prototype_request]()
    {
        core::trust::Request r
        {
            prototype_request.from,
            prototype_request.feature,
            prototype_request.when,
            prototype_request.answer
        };

        std::this_thread::sleep_for(std::chrono::milliseconds{500});

        auto store = core::trust::resolve_store_in_session_with_name(service_name);
        for (unsigned int i = 0; i < request_count; i++)
        {
            r.feature = i;
            EXPECT_NO_THROW(store->add(r));
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
