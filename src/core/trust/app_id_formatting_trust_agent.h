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

#ifndef CORE_TRUST_APP_ID_FORMATTING_TRUST_AGENT_H_
#define CORE_TRUST_APP_ID_FORMATTING_TRUST_AGENT_H_

#include <core/trust/agent.h>

namespace core
{
namespace trust
{
// An agent implementation pre-processing application ids, ensuring
// legible application ids, independent of application versions. Forwards
// to an actual agent implementation.
class CORE_TRUST_DLL_PUBLIC AppIdFormattingTrustAgent : public core::trust::Agent
{
public:
    AppIdFormattingTrustAgent(const std::shared_ptr<Agent>& impl);

    // From core::trust::Agent
    Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters) override;

private:
    std::shared_ptr<Agent> impl;
};
}
}

#endif // CORE_TRUST_APP_ID_FORMATTING_TRUST_AGENT_H_
