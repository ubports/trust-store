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

#ifndef CORE_TRUST_STORE_H_
#define CORE_TRUST_STORE_H_

#include <core/trust/request.h>
#include <core/trust/visibility.h>

#include <functional>
#include <memory>

namespace core
{
namespace trust
{
class CORE_TRUST_DLL_PUBLIC Store
{
public:
    struct Errors
    {
        Errors() = delete;
        ~Errors() = delete;

        struct ErrorOpeningStore : public std::runtime_error
        {
            ErrorOpeningStore(const char* implementation_specific)
                : std::runtime_error(implementation_specific)
            {
            }
        };

        struct ErrorResettingStore : public std::runtime_error
        {
            ErrorResettingStore(const char* implementation_specific)
                : std::runtime_error(implementation_specific)
            {
            }
        };

    };

    class Query
    {
    public:
        struct Error
        {
            struct QueryIsInErrorState : public std::runtime_error
            {
                QueryIsInErrorState() : std::runtime_error("Query is in error state, cannot extract result.")
                {
                }
            };

            struct NoCurrentResult : public std::runtime_error
            {
                NoCurrentResult() : std::runtime_error("Query does not have a current result.")
                {
                }
            };
        };

        enum class Status
        {
            armed, ///< The query is armed but hasn't been run.
            has_more_results, ///< The query has been executed and has more results.
            eor, ///< All results have been visited.
            error ///< An error occured.
        };

        Query(const Query&) = delete;
        virtual ~Query() = default;

        /** @brief Access the status of the query. */
        virtual Status status() const = 0;

        /** @brief Limit the query to a specific application Id. */
        virtual void for_application_id(const std::string& id) = 0;

        /** @brief Limit the query to an application-specific feature. */
        virtual void for_feature(unsigned int feature) = 0;

        /** @brief Limit the query to the specified time interval. */
        virtual void for_interval(const Request::Timestamp& begin, const Request::Timestamp& end) = 0;

        /** @brief Limit the query for a specific answer. */
        virtual void for_answer(Request::Answer answer) = 0;

        /** @brief Query all stored requests. */
        virtual void all() = 0;

        /** @brief Execute the query against the store. */
        virtual void execute() = 0;

        /** @brief After successful execution, advance to the next request. */
        virtual void next() = 0;

        /** @brief After successful execution, erase the current element and advance to the next request. */
        virtual void erase() = 0;

        /** @brief Access the request the query currently points to. */
        virtual Request current() = 0;

    protected:
        Query() = default;
    };

    Store(const Store&) = delete;
    virtual ~Store() = default;

    Store& operator=(const Store&) = delete;
    bool operator==(const Store&) const = delete;

    /** @brief Resets the state of the store, implementations should discard
     * all persistent and non-persistent state.
     */
    virtual void reset() = 0;

    /** @brief Add the provided request to the store. When this function returns true,
      * the request has been persisted by the implementation.
      */
    virtual void add(const Request& request) = 0;

    /**
     * @brief Create a query for this store.
     */
    virtual std::shared_ptr<Query> query() = 0;

protected:
    Store() = default;
};

CORE_TRUST_DLL_PUBLIC std::shared_ptr<Store> create_default_store();
}
}

#endif // CORE_TRUST_STORE_H_
