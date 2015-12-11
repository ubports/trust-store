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

#ifndef CORE_TRUST_MIR_CLICK_DESKTOP_ENTRY_APP_NAME_RESOLVER_H_
#define CORE_TRUST_MIR_CLICK_DESKTOP_ENTRY_APP_NAME_RESOLVER_H_

#include <core/trust/mir/agent.h>

namespace core
{
namespace trust
{
namespace mir
{
// A ClickDesktopEntryAppNameResolver queries the click database of installed
// packages to resolve an app's installation folder in the local filesystem.
// The directory is searched for a .desktop file, that is then loaded and queried 
// for the app's localized name.
class CORE_TRUST_DLL_PUBLIC ClickDesktopEntryAppInfoResolver : public AppInfoResolver
{
public:
    // ClickDesktopEntryAppNameResolver sets up an instance with default dbs.
    ClickDesktopEntryAppInfoResolver();

    // resolve queries the click index and an apps desktop file entry for 
    // obtaining a localized application name. Throws std::runtime_error in 
    // case of issues.
    AppInfo resolve(const std::string& app_id) override;
};
}
}
}

#endif // CORE_TRUST_MIR_CLICK_DESKTOP_ENTRY_APP_NAME_RESOLVER_H_
