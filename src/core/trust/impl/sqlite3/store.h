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
#ifndef CORE_TRUST_IMPL_SQLITE3_H_
#define CORE_TRUST_IMPL_SQLITE3_H_

#include <core/trust/visibility.h>

#include <memory>
#include <string>

namespace xdg
{
class BaseDirSpecification;
}

namespace core
{
namespace trust
{
class Store;
namespace impl
{
namespace sqlite
{
// create_for_service creates a Store implementation relying on sqlite3, managing
// trust for the service identified by service_name. Uses spec to determine a user-specific
// directory to place the trust database.
CORE_TRUST_DLL_PUBLIC std::shared_ptr<core::trust::Store> create_for_service(const std::string& service_name, xdg::BaseDirSpecification& spec);
}
}
}
}

#endif // CORE_TRUST_IMPL_SQLITE3_H_
