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

#include <core/trust/white_listing_agent.h>

#include "mock_agent.h"
#include "the.h"

#include <gmock/gmock.h>

namespace
{
std::shared_ptr<testing::NiceMock<MockAgent>> a_mocked_agent()
{
    return std::make_shared<testing::NiceMock<MockAgent>>();
}

struct MockWhiteListingPredicate
{
    core::trust::WhiteListingAgent::WhiteListingPredicate to_functional()
    {
        return [this](const core::trust::Agent::RequestParameters& params)
        {
            return is_whitelisted(params);
        };
    }

    MOCK_METHOD1(is_whitelisted, bool(const core::trust::Agent::RequestParameters&));
};
}

TEST(WhiteListingAgent, ctor_throws_for_null_agent)
{
    EXPECT_ANY_THROW(core::trust::WhiteListingAgent
    (
        core::trust::WhiteListingAgent::always_grant_for_unconfined(),
        std::shared_ptr<core::trust::Agent>()
    ));
}

TEST(WhiteListingAgent, invokes_predicate_for_incoming_request_and_dispatches_to_impl_for_non_whitelisted)
{
    using namespace ::testing;

    auto mock_agent = a_mocked_agent();

    auto params = the::default_request_parameters_for_testing();
    params.application.id = params.application.id + std::string{"_app"} + std::string{"_1.2.3"};

    MockWhiteListingPredicate wlp;
    EXPECT_CALL(wlp, is_whitelisted(params))
            .Times(1)
            .WillRepeatedly(Return(false));

    EXPECT_CALL(*mock_agent, authenticate_request_with_parameters(params))
            .Times(1)
            .WillRepeatedly(Return(core::trust::Request::Answer::denied));

    core::trust::WhiteListingAgent agent{wlp.to_functional(), mock_agent};

    EXPECT_EQ(core::trust::Request::Answer::denied,
              agent.authenticate_request_with_parameters(params));
}

TEST(WhiteListingAgent, invokes_predicate_for_incoming_request_and_returns_immediately_for_non_whitelisted)
{
    using namespace ::testing;

    auto mock_agent = a_mocked_agent();

    auto params = the::default_request_parameters_for_testing();
    params.application.id = params.application.id + std::string{"_app"} + std::string{"_1.2.3"};

    MockWhiteListingPredicate wlp;
    EXPECT_CALL(wlp, is_whitelisted(params))
            .Times(1)
            .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_agent, authenticate_request_with_parameters(params))
            .Times(0);

    core::trust::WhiteListingAgent agent{wlp.to_functional(), mock_agent};

    EXPECT_EQ(core::trust::Request::Answer::granted,
              agent.authenticate_request_with_parameters(params));
}

TEST(WhiteListingAgent, unconfined_predicate_only_returns_true_for_unconfined)
{
    using namespace ::testing;

    auto predicate = core::trust::WhiteListingAgent::always_grant_for_unconfined();

    auto params = the::default_request_parameters_for_testing();
    params.application.id = params.application.id + std::string{"_app"} + std::string{"_1.2.3"};

    EXPECT_FALSE(predicate(params));
    params.application.id = "unconfined";
    EXPECT_TRUE(predicate(params));
}
