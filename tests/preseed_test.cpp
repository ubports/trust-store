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

#include <core/trust/preseed.h>

#include "test_data.h"

#include <core/posix/exec.h>
#include <core/posix/this_process.h>

#include <gtest/gtest.h>

#include <map>

namespace
{
std::map<std::string, std::string> a_copy_of_the_current_env()
{
    std::map<std::string, std::string> result;

    core::posix::this_process::env::for_each([&result](const std::string& s, const std::string& t)
    {
        result.insert(std::make_pair(s, t));
    });

    return result;
}
}

TEST(Preseed, correctly_populates_per_service_trust_store)
{
    const std::string service_name{"JustANameForTesting"};
    const std::string app_name{"does.not.exist.app"};

    const std::vector<std::string> argv
    {
        "--for-service", service_name,
        "--request", "does.not.exist.app 0 granted",     // 1
        "--request", "does.not.exist.app 1 denied",      // 2
        "--request", "does.not.exist.app 2 granted",     // 3
        "--request", "does.not.exist.app 3 denied",      // 4
        "--request", "does.not.exist.app 4 granted",     // 5
        "--request", "does.not.exist.app 5 denied",      // 6
        "--request", "does.not.exist.app 6 granted",     // 7
        "--request", "does.not.exist.app 7 denied"      // 8
    };

    auto child = core::posix::exec(
                core::trust::testing::trust_store_preseed_executable_in_build_dir,
                argv,
                a_copy_of_the_current_env(),
                core::posix::StandardStream::empty);

    auto result = child.wait_for(core::posix::wait::Flags::untraced);

    EXPECT_EQ(core::posix::wait::Result::Status::exited, result.status);
    EXPECT_EQ(core::posix::exit::Status::success, result.detail.if_exited.status);

    auto store = core::trust::create_default_store(service_name);

    auto query = store->query();

    EXPECT_NO_THROW(query->execute());

    std::size_t counter{0};

    while (query->status() != core::trust::Store::Query::Status::eor)
    {
        EXPECT_EQ(counter, query->current().feature.value);

        if (counter % 2 == 0)
            EXPECT_EQ(core::trust::Request::Answer::granted, query->current().answer);
        else
            EXPECT_EQ(core::trust::Request::Answer::denied, query->current().answer);

        EXPECT_EQ(app_name, query->current().from);

        query->next(); counter++;
    }
}
