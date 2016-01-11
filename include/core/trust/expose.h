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

#ifndef CORE_TRUST_EXPOSE_H_
#define CORE_TRUST_EXPOSE_H_

#include <core/trust/visibility.h>

#include <memory>
#include <string>

namespace core
{
namespace dbus
{
class Bus;
}
namespace trust
{
// Forward declarations
class Store;

/** @brief Opaque type describing the exposure of a store instance.*/
class CORE_TRUST_DLL_PUBLIC Token
{
public:
    Token(const Token&) = delete;
    virtual ~Token() = default;

    Token& operator=(const Token&) = delete;
    bool operator==(const Token&) const = delete;

protected:
    Token() = default;
};

/**
 * @brief Exposes an existing store instance on the given bus.
 * @throw Error::ServiceNameMustNotBeEmpty.
 * @param store The instance to be exposed.
 * @param bus The bus connection.
 * @param name The name under which the service can be found within the session.
 * @return A token that limits the lifetime of the exposure.
 */
CORE_TRUST_DLL_PUBLIC std::unique_ptr<Token> expose_store_to_bus_with_name(
        const std::shared_ptr<Store>& store,
        const std::shared_ptr<core::dbus::Bus>& bus,
        const std::string& name);

/**
 * @brief Exposes an existing store instance with the current user session.
 * @throw Error::ServiceNameMustNotBeEmpty.
 * @param store The instance to be exposed.
 * @param name The name under which the service can be found within the session.
 * @return A token that limits the lifetime of the exposure.
 */
CORE_TRUST_DLL_PUBLIC std::unique_ptr<Token> expose_store_to_session_with_name(
        const std::shared_ptr<Store>& store,
        const std::string& name);
}
}

#endif // CORE_TRUST_EXPOSE_H_
