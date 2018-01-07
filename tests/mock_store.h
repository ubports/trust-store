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

#ifndef MOCK_STORE_H_
#define MOCK_STORE_H_

#include <core/trust/request.h>
#include <core/trust/store.h>

#include <gmock/gmock.h>

struct MockStore : public core::trust::Store
{
    struct MockQuery : public core::trust::Store::Query
    {
        /** @brief Access the status of the query. */
        MOCK_CONST_METHOD0(status, core::trust::Store::Query::Status());

        /** @brief Limit the query to a specific application Id. */
        MOCK_METHOD1(for_application_id, void(const std::string&));

        /** @brief Limit the query to a service-specific feature. */
        MOCK_METHOD1(for_feature, void(core::trust::Feature));

        /** @brief Limit the query to the specified time interval. */
        MOCK_METHOD2(for_interval, void(const core::trust::Request::Timestamp&, const core::trust::Request::Timestamp&));

        /** @brief Limit the query for a specific answer. */
        MOCK_METHOD1(for_answer, void(core::trust::Request::Answer));

        /** @brief Query all stored requests. */
        MOCK_METHOD0(all, void());

        /** @brief Execute the query against the store. */
        MOCK_METHOD0(execute, void());

        /** @brief After successful execution, advance to the next request. */
        MOCK_METHOD0(next, void());

        /** @brief After successful execution, erase the current element and advance to the next request. */
        MOCK_METHOD0(erase, void());

        /** @brief Access the request the query currently points to. */
        MOCK_METHOD0(current, core::trust::Request());
    };

    /** @brief Resets the state of the store, implementations should discard
     * all persistent and non-persistent state.
     */
    MOCK_METHOD0(reset, void());

    /** @brief Add the provided request to the store. When this function returns true,
      * the request has been persisted by the implementation.
      */
    MOCK_METHOD1(add, void(const core::trust::Request&));

    /** @brief Remove all records about the given application.
      */
    MOCK_METHOD1(remove_application, void(const std::string&));

    /**
     * @brief Create a query for this store.
     */
    MOCK_METHOD0(query, std::shared_ptr<core::trust::Store::Query>());
};

#endif // MOCK_STORE_H_
