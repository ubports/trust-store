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

#include <core/trust/store.h>

#include <gtest/gtest.h>

namespace
{
static const std::string service_name{"52EB2494-76F2-4ABB-A188-20B2D5B3CC94"};
}

// See https://bugs.launchpad.net/ubuntu-rtm/+source/trust-store/+bug/1387734
TEST(TrustStore, cached_user_replies_are_sorted_by_age_in_descending_order)
{
    auto store = core::trust::create_default_store(service_name);
    store->reset();

    const std::string app1{"com.does.not.exist.app1"};

    // This is the older reply, the user answered granted.
    core::trust::Request r1
    {
        app1,
        core::trust::Feature{0},
        std::chrono::system_clock::time_point(std::chrono::seconds{0}),
        core::trust::Request::Answer::granted
    };

    // This is the newer reply, the user revoked the trust.
    core::trust::Request r2
    {
        app1,
        core::trust::Feature{0},
        std::chrono::system_clock::time_point(std::chrono::seconds{500}),
        core::trust::Request::Answer::denied
    };

    store->add(r1);
    store->add(r2);

    auto query = store->query();
    query->execute();

    EXPECT_EQ(core::trust::Store::Query::Status::has_more_results, query->status());
    // We expect the more recent query to be enumerated first.
    EXPECT_EQ(r2, query->current()); query->next();
    // We expect the older query to be enumerated second.
    EXPECT_EQ(r1, query->current()); query->next();
    EXPECT_EQ(core::trust::Store::Query::Status::eor, query->status());
}
