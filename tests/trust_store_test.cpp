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

#include <core/trust/store.h>

#include <gtest/gtest.h>

#include <thread>

namespace
{
static const std::string service_name{"52EB2494-76F2-4ABB-A188-20B2D5B3CC94"};
}

TEST(TrustStoreRequestAnswer, is_printed_correctly)
{
    {
        std::stringstream ss; ss << core::trust::Request::Answer::granted;
        EXPECT_EQ("granted", ss.str());
    }
    {
        std::stringstream ss; ss << core::trust::Request::Answer::denied;
        EXPECT_EQ("denied", ss.str());
    }
}

TEST(TrustStoreRequest, is_printed_correctly)
{
    core::trust::Request r
    {
        "this.does.not.exist.app",
        core::trust::Feature{0},
        std::chrono::system_clock::time_point(std::chrono::seconds{0}),
        core::trust::Request::Answer::granted
    };

    std::stringstream ss; ss << r;
    EXPECT_EQ("Request(from: this.does.not.exist.app, feature: 0, when: 0, answer: granted)",
              ss.str());
}

TEST(TrustStore, default_implementation_is_available)
{
    auto store = core::trust::create_default_store(service_name);
    EXPECT_TRUE(store != nullptr);
}

TEST(TrustStore, resetting_the_store_purges_requests)
{
    auto store = core::trust::create_default_store(service_name);

    store->reset();

    auto query = store->query();
    EXPECT_EQ(core::trust::Store::Query::Status::armed,
              query->status());

    query->all();

    EXPECT_NO_THROW(query->execute());

    EXPECT_EQ(core::trust::Store::Query::Status::eor,
              query->status());
}

TEST(TrustStore, added_requests_are_found_by_query)
{
    auto store = core::trust::create_default_store(service_name);

    store->reset();

    static const std::uint64_t base_feature = 0;

    core::trust::Request r1
    {
        "this.does.not.exist.app",
        core::trust::Feature{base_feature},
        std::chrono::system_clock::now(),
        core::trust::Request::Answer::granted
    };

    core::trust::Request r2 = r1;
    r2.feature.value = base_feature + 1;

    core::trust::Request r3 = r2;
    r2.feature.value = base_feature + 2;

    store->add(r1);
    store->add(r2);
    store->add(r3);

    auto query = store->query();
    query->all();

    query->execute();

    EXPECT_EQ(core::trust::Store::Query::Status::has_more_results, query->status());
    EXPECT_EQ(r1, query->current()); query->next();
    EXPECT_EQ(r2, query->current()); query->next();
    EXPECT_EQ(r3, query->current()); query->next();
    EXPECT_EQ(core::trust::Store::Query::Status::eor,
              query->status());
}

TEST(TrustStore, limiting_query_to_app_id_returns_correct_results)
{
    auto store = core::trust::create_default_store(service_name);
    store->reset();

    const std::string app1{"com.does.not.exist.app1"};
    const std::string app2{"com.does.not.exist.app2"};

    core::trust::Request r1
    {
        app1,
        core::trust::Feature{0},
        std::chrono::system_clock::now(),
        core::trust::Request::Answer::granted
    };

    core::trust::Request r2
    {
        app2,
        core::trust::Feature{0},
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
}

TEST(TrustStore, limiting_query_to_feature_returns_correct_results)
{
    auto store = core::trust::create_default_store(service_name);
    store->reset();

    const std::string app1{"com.does.not.exist.app1"};

    core::trust::Request r1
    {
        app1,
        core::trust::Feature{0},
        std::chrono::system_clock::now(),
        core::trust::Request::Answer::granted
    };

    core::trust::Request r2
    {
        app1,
        core::trust::Feature{1},
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
}

TEST(TrustStore, limiting_query_to_answer_returns_correct_results)
{
    auto store = core::trust::create_default_store(service_name);
    store->reset();

    const std::string app1{"com.does.not.exist.app1"};

    core::trust::Request r1
    {
        app1,
        core::trust::Feature{0},
        std::chrono::system_clock::now(),
        core::trust::Request::Answer::granted
    };

    core::trust::Request r2
    {
        app1,
        core::trust::Feature{1},
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
}

TEST(TrustStore, limiting_query_to_time_interval_returns_correct_result)
{
    auto store = core::trust::create_default_store(service_name);
    store->reset();

    const std::string app1{"com.does.not.exist.app1"};

    core::trust::Request r1
    {
        app1,
        core::trust::Feature{0},
        std::chrono::system_clock::time_point(std::chrono::seconds{0}),
        core::trust::Request::Answer::granted
    };

    core::trust::Request r2
    {
        app1,
        core::trust::Feature{1},
        std::chrono::system_clock::time_point(std::chrono::seconds{500}),
        core::trust::Request::Answer::granted
    };

    core::trust::Request r3
    {
        app1,
        core::trust::Feature{1},
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
}

TEST(TrustStore, limiting_query_to_time_interval_and_answer_returns_correct_result)
{
    auto store = core::trust::create_default_store(service_name);
    store->reset();

    const std::string app1{"com.does.not.exist.app1"};

    core::trust::Request r1
    {
        app1,
        core::trust::Feature{0},
        std::chrono::system_clock::time_point(std::chrono::seconds{0}),
        core::trust::Request::Answer::granted
    };

    core::trust::Request r2
    {
        app1,
        core::trust::Feature{1},
        std::chrono::system_clock::time_point(std::chrono::seconds{500}),
        core::trust::Request::Answer::granted
    };

    core::trust::Request r3
    {
        app1,
        core::trust::Feature{1},
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
}

TEST(TrustStore, added_requests_are_found_by_query_multi_threaded)
{
    auto store = core::trust::create_default_store(service_name);

    store->reset();

    static const std::uint64_t base_feature = 0;

    auto inserter = [store](int base)
    {
        core::trust::Request r
        {
            "this.does.not.exist.app",
            core::trust::Feature{base_feature},
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::granted
        };

        for (unsigned int i = 0; i < 100; i++)
        {
            r.feature.value = base + i;
            store->add(r);
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
}

TEST(TrustStore, erasing_requests_empties_store)
{
    auto store = core::trust::create_default_store(service_name);
    store->reset();

    // Insert a bunch of requests and erase them after that.
    {
        core::trust::Request r
        {
            "this.does.not.exist.app",
            core::trust::Feature{0},
            std::chrono::system_clock::now(),
            core::trust::Request::Answer::granted
        };

        for (unsigned int i = 0; i < 100; i++)
        {
            r.feature.value = i;
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
}
