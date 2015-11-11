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

#include <core/trust/mir/click_desktop_entry_app_name_resolver.h>

#include "test_data.h"

#include <gtest/gtest.h>

namespace mir = core::trust::mir;

namespace
{
std::string click_db_for_testing()
{
    return core::trust::testing::current_source_dir + std::string("/click_db_for_testing");
}
}

TEST(ClickDesktopEntryAppNameResolver, throws_for_invalid_app_id)
{
    mir::ClickDesktopEntryAppNameResolver resolver;
    EXPECT_THROW(resolver.resolve(""), std::runtime_error);
    EXPECT_THROW(resolver.resolve("something:not:right"), std::runtime_error);
}

TEST(ClickDesktopEntryAppNameResolver, works_for_valid_app_and_app_id)
{
    mir::ClickDesktopEntryAppNameResolver resolver{{click_db_for_testing()}};
    EXPECT_NO_THROW(resolver.resolve("valid.pkg_app_0.0.0"));
    EXPECT_NO_THROW(resolver.resolve("valid.alt.pkg_app_0.0.0"));
}

TEST(ClickDesktopEntryAppNameResolver, throws_for_pkg_missing_desktop_entry)
{
    mir::ClickDesktopEntryAppNameResolver resolver{{click_db_for_testing()}};
    EXPECT_THROW(resolver.resolve("invalid.pkg_app_0.0.0"), std::runtime_error);
}
