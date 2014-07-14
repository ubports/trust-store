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

#ifndef CORE_TRUST_MIR_AGENT_H_
#define CORE_TRUST_MIR_AGENT_H_

#include <memory>

// Forward declare the MirConnection type.
struct MirConnection;

namespace core
{
namespace trust
{
// Forward declare the Agent interface.
class Agent;

namespace mir
{
/**
 * @brief create_agent_for_mir_connection creates a trust::Agent implementation leveraging Mir's trusted prompting API.
 * @param connection An existing connection to a Mir instance.
 * @throws std::logic_error if the connection object is NULL.
 */
std::shared_ptr<core::trust::Agent> create_agent_for_mir_connection(MirConnection* connection);
}
}
}

#endif // CORE_TRUST_MIR_AGENT_H_
