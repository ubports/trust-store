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

#ifndef CORE_TRUST_WHITE_LISTING_AGENT_H_
#define CORE_TRUST_WHITE_LISTING_AGENT_H_

#include <core/trust/agent.h>
#include <functional>

namespace core
{
namespace trust
{
// An agent implementation that allows for selectively whitelisting app ids.
class CORE_TRUST_DLL_PUBLIC WhiteListingAgent : public core::trust::Agent
{
public:
    // A functor that is evaluated for every incoming requests.
    // If it returns true, the request is immediately granted, otherwise passed on
    // to the next agent.
    typedef std::function<bool(const RequestParameters&)> WhiteListingPredicate;

    // Returns a predicate that returns true iff the app id is 'unconfined'.
    static WhiteListingPredicate always_grant_for_unconfined();

    WhiteListingAgent(WhiteListingPredicate white_listing_predicate, const std::shared_ptr<Agent>& impl);

    // From core::trust::Agent
    Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters) override;

private:
    WhiteListingPredicate white_listing_predicate;
    std::shared_ptr<Agent> impl;
};
}
}

#endif // CORE_TRUST_WHITE_LISTING_AGENT_H_
