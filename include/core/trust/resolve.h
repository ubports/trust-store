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

#ifndef CORE_TRUST_RESOLVE_H_
#define CORE_TRUST_RESOLVE_H_

#include <core/trust/visibility.h>

#include <memory>
#include <string>

namespace core
{
namespace trust
{
// Forward declarations
class Store;

/**
 * @brief Resolves an existing store instance within the current user session.
 * @throw Error::ServiceNameMustNotBeEmpty.
 * @param name The name under which the service can be found within the session.
 * @return A token that limits the lifetime of the exposure.
 */
CORE_TRUST_DLL_PUBLIC std::shared_ptr<Store> resolve_store_in_session_with_name(
        const std::string& name);
}
}

#endif // CORE_TRUST_RESOLVE_H_
