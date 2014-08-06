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

#include <core/trust/store.h>

core::trust::CachedAgent::CachedAgent(const core::trust::CachedAgent::Configuration& configuration)
    : configuration(configuration)
{
    // We verify parameters first:
    if (not configuration.agent) throw std::logic_error
    {
        "Cannot operate without an agent implementation."
    };

    if (not configuration.store) throw std::logic_error
    {
        "Cannot operate without a store implementation."
    };
}

// From core::trust::Agent
core::trust::Request::Answer core::trust::CachedAgent::authenticate_request_with_parameters(
        const core::trust::Agent::RequestParameters& params)
{
    // Let's see if the store has an answer for app-id and feature.
    auto query = configuration.store->query();

    // Narrow it down to the specific app and the specific feature
    query->for_application_id(params.application.id);
    query->for_feature(params.feature);

    query->execute();

    // We have got results and we take the most recent one as the most appropriate.
    if (query->status() == core::trust::Store::Query::Status::has_more_results)
    {
        // And we are returning early.
        return query->current().answer;
    }

    // We do not have results available in the store, prompting the user
    auto answer = configuration.agent->authenticate_request_with_parameters(
                core::trust::Agent::RequestParameters
                {
                    params.application.uid,
                    params.application.pid,
                    params.application.id,
                    params.feature,
                    params.description
                });

    configuration.store->add(core::trust::Request
    {
        params.application.id,
        params.feature,
        std::chrono::system_clock::now(),
        answer
    });

    return answer;
}
