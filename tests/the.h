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

// Concrete values for reuse in testing
namespace the
{
core::trust::Pid default_pid_for_testing()
{
    return core::trust::Pid{42};
}

core::trust::Uid default_uid_for_testing()
{
    return core::trust::Uid{42};
}

core::trust::Feature default_feature_for_testing()
{
    return core::trust::Feature{0};
}

core::trust::Agent::RequestParameters default_request_parameters_for_testing()
{
    return core::trust::Agent::RequestParameters
    {
        default_uid_for_testing(),
        default_pid_for_testing(),
        "this.is.just.for.testing.purposes",
        default_feature_for_testing(),
        "Someone wants to access all your credentials and steal your identity."
    };
}
}
