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

#ifndef CORE_TRUST_DBUS_INTERFACE_H_
#define CORE_TRUST_DBUS_INTERFACE_H_

#include <core/trust/request.h>
#include <core/trust/store.h>

#include <org/freedesktop/dbus/traits/service.h>

#include <chrono>
#include <string>

namespace org
{
namespace freedesktop
{
namespace dbus
{
namespace traits
{
template<>
struct Service<core::trust::Store>
{
    static std::string interface_name()
    {
        return "core.trust.Store";
    }
};
}
}
}
}

namespace core
{
namespace trust
{
struct DBusInterface
{
    static std::string& mutable_name()
    {
        static std::string s;
        return s;
    }

    static const std::string& name()
    {
        return mutable_name();
    }

    struct Error
    {
        struct AddingRequest
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.trust.store.error.AddingRequest"
                };

                return s;
            }
            typedef core::trust::Store Interface;
        };

        struct ResettingStore
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.trust.store.error.ResettingStore"
                };

                return s;
            }
            typedef core::trust::Store Interface;
        };
    };

    struct Add
    {
        inline static const std::string& name()
        {
            static const std::string& s
            {
                "Add"
            };
            return s;
        }
        typedef core::trust::Store Interface;
        typedef core::trust::Request ArgumentType;
        typedef void ResultType;

        inline static const std::chrono::milliseconds default_timeout()
        {
            return std::chrono::seconds{1};
        }
    };

    struct Reset
    {
        inline static const std::string& name()
        {
            static const std::string& s
            {
                "Reset"
            };
            return s;
        }
        typedef core::trust::Store Interface;
        typedef void ArgumentType;
        typedef void ResultType;

        inline static const std::chrono::milliseconds default_timeout()
        {
            return std::chrono::seconds{1};
        }
    };
};
}
}

#endif // CORE_TRUST_DBUS_INTERFACE_H_
