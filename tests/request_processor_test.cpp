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

#include <core/trust/agent.h>
#include <core/trust/request.h>
#include <core/trust/store.h>

#include "mock_agent.h"
#include "mock_store.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>

namespace
{
core::trust::Pid the_default_pid_for_testing()
{
    return core::trust::Pid{42};
}

core::trust::Uid the_default_uid_for_testing()
{
    return core::trust::Uid{42};
}

core::trust::Feature the_default_feature_for_testing()
{
    return core::trust::Feature{0};
}

std::shared_ptr<core::trust::Agent> a_null_agent()
{
    return std::shared_ptr<core::trust::Agent>{};
}

std::shared_ptr<testing::NiceMock<MockAgent>> a_mocked_agent()
{
    return std::make_shared<testing::NiceMock<MockAgent>>();
}

std::shared_ptr<core::trust::Store> a_null_store()
{
    return std::shared_ptr<core::trust::Store>{};
}

std::shared_ptr<testing::NiceMock<MockStore>> a_mocked_store()
{
    return std::make_shared<testing::NiceMock<MockStore>>();
}

std::shared_ptr<testing::NiceMock<MockStore::MockQuery>> a_mocked_query()
{
    return std::make_shared<testing::NiceMock<MockStore::MockQuery>>();
}

core::trust::RequestParameters default_request_parameters_for_testing()
{
    return core::trust::RequestParameters
    {
        a_null_agent(),
        a_null_store(),
        the_default_uid_for_testing(),
        the_default_pid_for_testing(),
        "this.is.just.for.testing.purposes",
        the_default_feature_for_testing(),
        "Someone wants to access all your credentials and steal your identity."
    };
}

core::trust::Request::Answer throw_a_dice()
{
    // We seed the rng with the current time to ensure randomness across test runs.
    static std::default_random_engine generator
    {
        static_cast<long unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())
    };
    // Our dice :)
    static std::uniform_int_distribution<int> distribution
    {
        1,
        6
    };

    return distribution(generator) <= 3 ?
                core::trust::Request::Answer::denied :
                core::trust::Request::Answer::granted;
}

}

TEST(RequestProcessing, throws_for_missing_agent_implementation)
{
    auto params = default_request_parameters_for_testing();

    params.store = a_mocked_store();

    EXPECT_THROW(core::trust::process_trust_request(params), std::logic_error);
}

TEST(RequestProcessing, throws_for_missing_store_implementation)
{
    auto params = default_request_parameters_for_testing();

    params.agent = a_mocked_agent();

    EXPECT_THROW(core::trust::process_trust_request(params), std::logic_error);
}

TEST(RequestProcessing, queries_store_for_cached_results_and_returns_cached_value)
{
    using namespace ::testing;

    auto answer = throw_a_dice();

    auto params = default_request_parameters_for_testing();

    core::trust::Request request
    {
        params.application_id,
        params.feature,
        std::chrono::system_clock::now(),
        answer
    };

    auto mocked_agent = a_mocked_agent();
    auto mocked_query = a_mocked_query();
    auto mocked_store = a_mocked_store();

    ON_CALL(*mocked_query, status())
            .WillByDefault(
                Return(
                    core::trust::Store::Query::Status::has_more_results));

    ON_CALL(*mocked_query, current())
            .WillByDefault(
                Return(
                    request));

    ON_CALL(*mocked_store, query())
            .WillByDefault(
                Return(
                    mocked_query));

    EXPECT_CALL(*mocked_store, query()).Times(1);
    // We expect the processor to limit the query to the respective application id
    // and to the respective feature.
    EXPECT_CALL(*mocked_query, for_application_id(params.application_id)).Times(1);
    EXPECT_CALL(*mocked_query, for_feature(params.feature)).Times(1);
    // The setup ensures that a previously stored answer is available in the store.
    // For that, the agent should not be queried.
    EXPECT_CALL(*mocked_agent, authenticate_request_with_parameters(_)).Times(0);

    params.agent = mocked_agent;
    params.store = mocked_store;

    EXPECT_EQ(answer, core::trust::process_trust_request(params));
}

TEST(RequestProcessing, queries_agent_if_no_cached_results_and_returns_users_answer)
{
    using namespace ::testing;

    auto answer = throw_a_dice();

    auto params = default_request_parameters_for_testing();

    core::trust::Agent::RequestParameters agent_params
    {
        params.application_uid,
        params.application_pid,
        params.application_id,
        params.feature,
        params.description
    };

    core::trust::Request request
    {
        params.application_id,
        params.feature,
        std::chrono::system_clock::now(),
        answer
    };

    auto mocked_agent = a_mocked_agent();
    auto mocked_query = a_mocked_query();
    auto mocked_store = a_mocked_store();

    ON_CALL(*mocked_agent, authenticate_request_with_parameters(agent_params))
            .WillByDefault(
                Return(
                    answer));

    // We return EndOfRecord for queries, and expect the request processor
    // to subsequently ask the user for his answer.
    ON_CALL(*mocked_query, status())
            .WillByDefault(
                Return(
                    core::trust::Store::Query::Status::eor));

    ON_CALL(*mocked_store, query())
            .WillByDefault(
                Return(
                    mocked_query));

    EXPECT_CALL(*mocked_query, current()).Times(0);
    EXPECT_CALL(*mocked_store, query()).Times(1);
    // We expect the processor to limit the query to the respective application id
    // and to the respective feature.
    EXPECT_CALL(*mocked_query, for_application_id(params.application_id)).Times(1);
    EXPECT_CALL(*mocked_query, for_feature(params.feature)).Times(1);
    // The setup ensures that a previously stored answer is available in the store.
    // For that, the agent should not be queried.
    EXPECT_CALL(*mocked_agent, authenticate_request_with_parameters(agent_params)).Times(1);

    params.agent = mocked_agent;
    params.store = mocked_store;

    EXPECT_EQ(answer, core::trust::process_trust_request(params));
}



