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

#include <core/trust/privilege_escalation_prevention_agent.h>

#include "mock_agent.h"
#include "the.h"

#include <gmock/gmock.h>

namespace
{
std::shared_ptr<testing::NiceMock<MockAgent>> a_mocked_agent()
{
    return std::make_shared<testing::NiceMock<MockAgent>>();
}

struct MockUserIdFunctor
{
    core::trust::PrivilegeEscalationPreventionAgent::UserIdFunctor to_functional()
    {
        return [this]()
        {
            return get_uid();
        };
    }

    MOCK_METHOD0(get_uid, core::trust::Uid());
};
}

TEST(PrivilegeEscalationPreventionAgent, ctor_throws_for_null_agent)
{
    EXPECT_ANY_THROW(core::trust::PrivilegeEscalationPreventionAgent
    (
        core::trust::PrivilegeEscalationPreventionAgent::default_user_id_functor(),
        std::shared_ptr<core::trust::Agent>()
    ));
}

TEST(PrivilegeEscalationPreventionAgent, queries_user_id_for_incoming_request_and_dispatches_to_impl_if_no_privilege_escalation_detected)
{
    using namespace ::testing;

    auto mock_agent = a_mocked_agent();

    auto params = the::default_request_parameters_for_testing();
    params.application.id = params.application.id + std::string{"_app"} + std::string{"_1.2.3"};

    MockUserIdFunctor uif;
    EXPECT_CALL(uif, get_uid())
            .Times(1)
            .WillRepeatedly(Return(params.application.uid));

    EXPECT_CALL(*mock_agent, authenticate_request_with_parameters(params))
            .Times(1)
            .WillRepeatedly(Return(core::trust::Request::Answer::denied));

    core::trust::PrivilegeEscalationPreventionAgent agent{uif.to_functional(), mock_agent};

    EXPECT_EQ(core::trust::Request::Answer::denied,
              agent.authenticate_request_with_parameters(params));
}

TEST(PrivilegeEscalationPreventionAgent, invokes_user_id_functor_for_incoming_request_and_throws_if_privilege_escalation_detected)
{
    using namespace ::testing;

    auto mock_agent = a_mocked_agent();

    auto params = the::default_request_parameters_for_testing();
    params.application.id = params.application.id + std::string{"_app"} + std::string{"_1.2.3"};

    MockUserIdFunctor uif;
    EXPECT_CALL(uif, get_uid())
            .Times(1)
            .WillRepeatedly(Return(core::trust::Uid{12}));

    EXPECT_CALL(*mock_agent, authenticate_request_with_parameters(params))
            .Times(0);

    core::trust::PrivilegeEscalationPreventionAgent agent{uif.to_functional(), mock_agent};

    EXPECT_THROW(agent.authenticate_request_with_parameters(params), core::trust::PrivilegeEscalationPreventionAgent::Error);
}

TEST(PrivilegeEscalationPreventionAgentDefaultUserIdFunctor, returns_current_user_id)
{
    auto f = core::trust::PrivilegeEscalationPreventionAgent::default_user_id_functor();
    EXPECT_EQ(core::trust::Uid(::getuid()), f());
}
