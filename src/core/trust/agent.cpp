/*
 * Copyright © 2014 Canonical Ltd.
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

bool core::trust::operator==(const core::trust::Agent::RequestParameters& lhs, const core::trust::Agent::RequestParameters& rhs)
{
    return std::tie(lhs.application_id, lhs.application_pid, lhs.application_uid, lhs.description, lhs.feature) ==
           std::tie(rhs.application_id, rhs.application_pid, rhs.application_uid, rhs.description, rhs.feature);
}
