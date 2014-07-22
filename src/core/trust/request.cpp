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

#include <core/trust/request.h>

#include <core/trust/agent.h>
#include <core/trust/store.h>

core::trust::Request::Answer core::trust::process_trust_request(const core::trust::RequestParameters& params)
{
    // We verify parameters first:
    if (not params.agent) throw std::logic_error
    {
        "Cannot operate without an agent implementation."
    };

    if (not params.store) throw std::logic_error
    {
        "Cannot operate without a store implementation."
    };

    // Let's see if the store has an answer for app-id and feature.
    auto query = params.store->query();

    // Narrow it down to the specific app and the specific feature
    query->for_application_id(params.application_id);
    query->for_feature(params.feature);

    query->execute();

    // We have got results and we take the most recent one as the most appropriate.
    if (query->status() == core::trust::Store::Query::Status::has_more_results)
    {
        // And we are returning early.
        return query->current().answer;
    }

    // We do not have results available in the store, prompting the user
    auto answer = params.agent->prompt_user_for_request(
                core::trust::Uid{params.application_uid},
                core::trust::Pid{params.application_pid},
                params.application_id,
                params.description);

    params.store->add(core::trust::Request
    {
        params.application_id,
        params.feature,
        std::chrono::system_clock::now(),
        answer
    });

    return answer;
}

bool core::trust::operator==(const core::trust::Request& lhs, const core::trust::Request& rhs)
{
    return lhs.from == rhs.from &&
           lhs.feature == rhs.feature &&
           lhs.when == rhs.when &&
           lhs.answer == rhs.answer;
}

std::ostream& core::trust::operator<<(std::ostream& out, const core::trust::Request::Answer& a)
{
    switch (a)
    {
    case core::trust::Request::Answer::granted: out << "granted"; break;
    case core::trust::Request::Answer::denied: out << "denied"; break;
    }

    return out;
}

std::ostream& core::trust::operator<<(std::ostream& out, const core::trust::Request& r)
{
    out << "Request("
        << "from: " << r.from << ", "
        << "feature: " << r.feature << ", "
        << "when: " << r.when.time_since_epoch().count() << ", "
        << "answer: " << r.answer << ")";

    return out;
}
