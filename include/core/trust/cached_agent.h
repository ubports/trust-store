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

#ifndef CORE_TRUST_CACHED_AGENT_H_
#define CORE_TRUST_CACHED_AGENT_H_

#include <core/trust/agent.h>

namespace core
{
namespace trust
{
/** @brief An agent implementation that uses a trust store instance to cache results. */
class CORE_TRUST_DLL_PUBLIC CachedAgent : public core::trust::Agent
{
public:
    /** @brief To safe some typing. */
    typedef std::shared_ptr<CachedAgent> Ptr;

    /** @brief Creation time parameters. */
    struct Configuration
    {
        /** @brief The actual agent implementation for prompting the user. */
        std::shared_ptr<Agent> agent;
        /** @brief The store caching user answers to trust prompts. */
        std::shared_ptr<Store> store;
    };

    /**
     * @brief CachedAgent creates a new agent instance.
     * @param configuration Specificies the actual agent and the store.
     * @throws std::logic_error if either the agent or the store are null.
     */
    CachedAgent(const Configuration& configuration);
    /** @cond */
    virtual ~CachedAgent() = default;
    /** @endcond */

    /** @brief From core::trust::Agent. */
    Request::Answer authenticate_request_with_parameters(const core::trust::Agent::RequestParameters& parameters) override;

private:
    /** @brief We just store a copy of the configuration parameters */
    Configuration configuration;
};
}
}

#endif // CORE_TRUST_CACHED_AGENT_H_
