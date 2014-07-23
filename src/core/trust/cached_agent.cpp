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

#include <core/trust/cached_agent.h>

core::trust::CachedAgent::CachedAgent(const core::trust::CachedAgent::Configuration& configuration)
    : configuration(configuration)
{
}

// From core::trust::Agent
core::trust::Request::Answer core::trust::CachedAgent::authenticate_request_with_parameters(
        const core::trust::Agent::RequestParameters&)
{
    return core::trust::Request::Answer::denied;
}
