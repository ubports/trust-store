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

#include <click.h>
#include <glib.h>

#include <boost/format.hpp>

#include <memory>
#include <regex>
#include <stdexcept>

namespace mir = core::trust::mir;

std::string mir::ClickDesktopAppNameResolver::resolve(const std::string& app_id)
{
    // We translate to human readable strings here, and do it a non-translateable way first
    // We post-process the application id and try to extract the unversioned package name.
    // Please see https://wiki.ubuntu.com/AppStore/Interfaces/ApplicationId.
    static const std::regex regex_full_app_id{"(.*)_(.*)_(.*)"};
    static constexpr std::size_t index_pkg{1};
    static constexpr std::size_t index_app{2};
    static constexpr std::size_t index_version{3};

    std::smatch match;
    if (not std::regex_match(app_id, match, regex_full_app_id)) throw std::runtime_error
    {
        "Failed to parse application id " + app_id
    };

    // Wrap up a GError with an RAII approach, easing
    // cleanup if we throw an exception.
    struct Error
    {
        ~Error() { clear(); }
        void clear() { g_clear_error(&error); }

        GError* error = nullptr;
    } g;

    std::shared_ptr<ClickDB> click_db{click_db_new(), [](ClickDB* db) { if (db) click_db_gc(db, nullptr); }};
    
    if (not click_db) throw std::runtime_error
    {
        "Failed to load click db"
    };

    std::string path = click_db_get_path(click_db.get(), match.str(index_pkg).c_str(), match.str(index_version).c_str(), &g.error);

    if (g.error) throw std::runtime_error
    {
        "Failed to query click db [" + std::string(g.error->message) + "]"
    };

    g.clear();

    boost::format desktop_entry_pattern("%1%/share/applications/%2%.desktop");
    desktop_entry_pattern = desktop_entry_pattern % path % match.str(index_app);

    std::shared_ptr<GKeyFile> key_file{g_key_file_new(), [](GKeyFile* file) { if (file) g_key_file_free(file); }};

    if (not g_key_file_load_from_file(key_file.get(), desktop_entry_pattern.str().c_str(), G_KEY_FILE_NONE, &g.error)) throw std::runtime_error
    {
        "Failed to load desktop entry [" + std::string(g.error->message) + "]"
    };

    auto app_name = g_key_file_get_locale_string(key_file.get(), G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, nullptr, &g.error);

    if (g.error) throw std::runtime_error
    {
        "Failed to query localized name [" + std::string(g.error->message) + "]"
    };

    return std::string(app_name);
}
