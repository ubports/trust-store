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

#include <core/dbus/traits/service.h>
#include <core/dbus/types/object_path.h>

#include <chrono>
#include <string>

namespace core
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

template<>
struct Service<core::trust::Store::Query>
{
    static std::string interface_name()
    {
        return "core.trust.Store.Query";
    }
};
}
}
}

namespace core
{
namespace trust
{
namespace dbus
{
struct Store
{
    static const std::string& name()
    {
        static const std::string s{"com.ubuntu.trust.store"};
        return s;
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

        struct CreatingQuery
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.trust.store.error.CreatingQuery"
                };

                return s;
            }
            typedef core::trust::Store Interface;
        };
    };

    struct Query
    {
        struct Error
        {
            struct NoCurrentRequest
            {
                static const std::string& name()
                {
                    static const std::string s
                    {
                        "core.trust.store.query.error.NoCurrentRequest"
                    };

                    return s;
                }
                typedef core::trust::Store Interface;
            };
        };

        struct Status
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "Status"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef void ArgumentType;
            typedef core::trust::Store::Query::Status ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct ForApplicationId
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "ForApplicationId"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef std::string ArgumentType;
            typedef void ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct ForFeature
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "ForFeature"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef std::uint64_t ArgumentType;
            typedef void ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct ForInterval
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "ForInterval"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef std::tuple<std::int64_t, std::int64_t> ArgumentType;
            typedef void ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct ForAnswer
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "ForAnswer"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef core::trust::Request::Answer ArgumentType;
            typedef void ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct All
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "All"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef void ArgumentType;
            typedef void ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct Execute
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "Execute"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef void ArgumentType;
            typedef void ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct Next
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "Next"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef void ArgumentType;
            typedef void ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct Erase
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "Erase"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef void ArgumentType;
            typedef void ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
        };
        struct Current
        {
            inline static const std::string& name()
            {
                static const std::string& s
                {
                    "Current"
                };
                return s;
            }
            typedef core::trust::Store::Query Interface;
            typedef void ArgumentType;
            typedef core::trust::Request ResultType;

            inline static const std::chrono::milliseconds default_timeout()
            {
                return std::chrono::seconds{1};
            }
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

    struct AddQuery
    {
        inline static const std::string& name()
        {
            static const std::string& s
            {
                "AddQuery"
            };
            return s;
        }
        typedef core::trust::Store Interface;
        typedef void ArgumentType;
        typedef core::dbus::types::ObjectPath ResultType;

        inline static const std::chrono::milliseconds default_timeout()
        {
            return std::chrono::seconds{1};
        }
    };

    struct RemoveQuery
    {
        inline static const std::string& name()
        {
            static const std::string& s
            {
                "RemoveQuery"
            };
            return s;
        }
        typedef core::trust::Store Interface;
        typedef core::dbus::types::ObjectPath ArgumentType;
        typedef void ResultType;

        inline static const std::chrono::milliseconds default_timeout()
        {
            return std::chrono::seconds{1};
        }
    };
};
}
}
}

#endif // CORE_TRUST_DBUS_INTERFACE_H_
