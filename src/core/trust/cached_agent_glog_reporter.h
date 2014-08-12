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
#ifndef CORE_TRUST_CACHED_AGENT_GLOG_REPORTER_H_
#define CORE_TRUST_CACHED_AGENT_GLOG_REPORTER_H_

#include <core/trust/cached_agent.h>

namespace core
{
namespace trust
{
// Implements the CachedAgent::Reporter interface by leveraging Google Log.
class CORE_TRUST_DLL_PUBLIC CachedAgentGlogReporter : public CachedAgent::Reporter
{
public:
    // All creation time arguments go here.
    struct Configuration
    {
        // By default, the implementation reports to syslog.
        // If this flag is set to true, logging also goes to stderr.
        bool also_log_to_stderr{true};
    };

    // Creates a reporter instance with the given configuration, and initializes Google logging.
    CachedAgentGlogReporter(const Configuration& configuration);

    // Invoked whenever the implementation was able to resolve a cached request.
    void report_cached_answer_found(const core::trust::Request&);

    // Invoked whenever the implementation called out to an agent to prompt the user for trust.
    void report_user_prompted_for_trust(const core::trust::Request::Answer&);
};
}
}

#endif // CORE_TRUST_CACHED_AGENT_GLOG_REPORTER_H_
