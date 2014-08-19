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

#include <core/trust/app_id_formatting_trust_agent.h>

#include "mock_agent.h"
#include "the.h"

#include <gmock/gmock.h>

namespace
{
std::shared_ptr<testing::NiceMock<MockAgent>> a_mocked_agent()
{
    return std::make_shared<testing::NiceMock<MockAgent>>();
}
}

TEST(AppIdFormattingTrustAgent, ctor_throws_for_null_agent)
{
    EXPECT_ANY_THROW(core::trust::AppIdFormattingTrustAgent agent{std::shared_ptr<core::trust::Agent>()});
}

TEST(AppIdFormattingTrustAgent, removes_version_and_calls_to_implementation)
{
    using namespace ::testing;

    auto mock_agent = a_mocked_agent();

    auto params = the::default_request_parameters_for_testing();
    params.application.id = params.application.id + "_1.2.3";
    auto params_without_version = the::default_request_parameters_for_testing();

    EXPECT_CALL(*mock_agent,
                authenticate_request_with_parameters(params_without_version))
            .Times(1)
            .WillRepeatedly(Return(core::trust::Request::Answer::denied));

    core::trust::AppIdFormattingTrustAgent agent{mock_agent};

    EXPECT_EQ(core::trust::Request::Answer::denied,
              agent.authenticate_request_with_parameters(params));
}

TEST(AppIdFormattingTrustAgent, leaves_unversioned_app_id_as_is)
{
    using namespace ::testing;

    auto mock_agent = a_mocked_agent();

    auto params = the::default_request_parameters_for_testing();

    EXPECT_CALL(*mock_agent,
                authenticate_request_with_parameters(params))
            .Times(1)
            .WillRepeatedly(Return(core::trust::Request::Answer::denied));

    core::trust::AppIdFormattingTrustAgent agent{mock_agent};

    EXPECT_EQ(core::trust::Request::Answer::denied,
              agent.authenticate_request_with_parameters(params));
}
