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

#include <core/trust/white_listing_agent.h>

core::trust::WhiteListingAgent::WhiteListingPredicate core::trust::WhiteListingAgent::always_grant_for_unconfined()
{
    return [](const core::trust::Agent::RequestParameters& params) -> bool
    {
        return params.application.id == "unconfined";
    };
}

core::trust::WhiteListingAgent::WhiteListingAgent(
        core::trust::WhiteListingAgent::WhiteListingPredicate white_listing_predicate,
        const std::shared_ptr<core::trust::Agent>& impl)
    : white_listing_predicate{white_listing_predicate},
      impl{impl}
{
    if (not impl) throw std::runtime_error
    {
        "Missing agent implementation."
    };
}

core::trust::Request::Answer core::trust::WhiteListingAgent::authenticate_request_with_parameters(const core::trust::Agent::RequestParameters& parameters)
{
    if (white_listing_predicate(parameters))
        return core::trust::Request::Answer::granted;

    return impl->authenticate_request_with_parameters(parameters);
}
