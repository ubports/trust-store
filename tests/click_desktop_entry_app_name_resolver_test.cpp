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

#include <core/trust/mir/click_desktop_entry_app_info_resolver.h>
#include <core/posix/this_process.h>
#include "test_data.h"

#include <gtest/gtest.h>

#include <fstream>

namespace env = core::posix::this_process::env;
namespace mir = core::trust::mir;

namespace
{
bool ensure_icon_file()
{
    std::ofstream out{"/tmp/test.icon"};
    out << "test.icon";
    return out.good();
}
}

TEST(ClickDesktopEntryAppInfoResolver, throws_for_invalid_app_id)
{
    ASSERT_TRUE(ensure_icon_file());
    mir::ClickDesktopEntryAppInfoResolver resolver;
    EXPECT_THROW(resolver.resolve("something:not:right"), std::runtime_error);
}

TEST(ClickDesktopEntryAppInfoResolver, resolves_existing_desktop_entry_from_xdg_data_home)
{
    ASSERT_TRUE(ensure_icon_file());
    env::unset_or_throw("XDG_DATA_HOME");
    env::set_or_throw("XDG_DATA_HOME", core::trust::testing::current_source_dir + std::string("/share"));
    mir::ClickDesktopEntryAppInfoResolver resolver;
    EXPECT_NO_THROW(resolver.resolve("valid.pkg_app_0.0.0"));
}

TEST(ClickDesktopEntryAppInfoResolver, robustly_resolves_existing_desktop_entry_from_xdg_data_home)
{
    ASSERT_TRUE(ensure_icon_file());
    env::unset_or_throw("XDG_DATA_HOME");
    env::set_or_throw("XDG_DATA_HOME", core::trust::testing::current_source_dir + std::string("/share"));

    mir::ClickDesktopEntryAppInfoResolver resolver;
    EXPECT_NO_THROW(resolver.resolve("valid.pkg_app"));
}

TEST(ClickDesktopEntryAppInfoResolver, resolves_existing_desktop_entry_from_xdg_data_dirs)
{
    ASSERT_TRUE(ensure_icon_file());
    env::unset_or_throw("XDG_DATA_HOME"); env::unset_or_throw("XDG_DATA_DIRS");
    env::set_or_throw("XDG_DATA_HOME", core::trust::testing::current_source_dir + std::string("/empty"));
    env::set_or_throw("XDG_DATA_DIRS", core::trust::testing::current_source_dir + std::string("/share"));

    mir::ClickDesktopEntryAppInfoResolver resolver;
    resolver.resolve("valid.pkg_app_0.0.0");
}
