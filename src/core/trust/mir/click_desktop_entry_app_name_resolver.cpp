/*
 * Copyright © 2015 Canonical Ltd.
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

#include <glib.h>
#include <xdg.h>

#include <core/posix/this_process.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <memory>
#include <regex>
#include <string>
#include <stdexcept>
#include <vector>

namespace env = core::posix::this_process::env;
namespace fs = boost::filesystem;
namespace mir = core::trust::mir;

namespace
{
boost::filesystem::path resolve_desktop_entry_or_throw(const std::string& app_id)
{
    boost::format pattern("%1%/applications/%2%.desktop");
    fs::path p{(pattern % xdg::data().home().string() % app_id).str()};
    if (fs::is_regular_file(p))
        return p;
    
    fs::path applications{xdg::data().home() / "applications"};
    fs::directory_iterator it(applications), itE;
    while (it != itE)
    {
        if (it->path().filename().string().find(app_id) == 0)
            return it->path();
        ++it;
    }
    
    for (auto dir : xdg::data().dirs())
    {
        fs::path p{(pattern % dir.string() % app_id).str()};
        if (fs::is_regular_file(p))
            return p;
    }

    throw std::runtime_error{"Could not resolve desktop entry for " + app_id};
}

// Wrap up a GError with an RAII approach, easing
// cleanup if we throw an exception.
struct Error
{
    ~Error() { clear(); }
    void clear() { g_clear_error(&error); }

    GError* error = nullptr;
};

std::string name_from_desktop_entry_or_throw(const fs::path& fn)
{
    Error g;
    std::shared_ptr<GKeyFile> key_file{g_key_file_new(), [](GKeyFile* file) { if (file) g_key_file_free(file); }};

    if (not g_key_file_load_from_file(key_file.get(), fn.string().c_str(), G_KEY_FILE_NONE, &g.error)) throw std::runtime_error
    {
        "Failed to load desktop entry [" + std::string(g.error->message) + "]"
    };

    auto app_name = g_key_file_get_locale_string(key_file.get(), G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, nullptr, &g.error);

    if (g.error) throw std::runtime_error
    {
        "Failed to query localized name [" + std::string(g.error->message) + "]"
    };

    return app_name;
}
}

mir::ClickDesktopEntryAppNameResolver::ClickDesktopEntryAppNameResolver()
{
}

std::string mir::ClickDesktopEntryAppNameResolver::resolve(const std::string& app_id)
{
    return name_from_desktop_entry_or_throw(resolve_desktop_entry_or_throw(app_id));
}
