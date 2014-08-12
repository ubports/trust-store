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

#include <core/trust/cached_agent_glog_reporter.h>

#include <glog/logging.h>

namespace
{
struct Initializer
{
    Initializer()
    {
        google::InitGoogleLogging("core::trust::Daemon");
    }

    ~Initializer()
    {
        google::ShutdownGoogleLogging();
    }
} initializer;
}

// Creates a reporter instance with the given configuration, and initializes Google logging.
core::trust::CachedAgentGlogReporter::CachedAgentGlogReporter(const core::trust::CachedAgentGlogReporter::Configuration& configuration)
{
    FLAGS_alsologtostderr = configuration.also_log_to_stderr;
}

// Invoked whenever the implementation was able to resolve a cached request.
void core::trust::CachedAgentGlogReporter::report_cached_answer_found(const core::trust::Request& r)
{
    SYSLOG(INFO) << "CachedAgent::authenticate_request_with_parameters: Found cached answer " << r;
}

// Invoked whenever the implementation called out to an agent to prompt the user for trust.
void core::trust::CachedAgentGlogReporter::report_user_prompted_for_trust(const core::trust::Request::Answer& a)
{
    SYSLOG(INFO) << "CachedAgent::authenticate_request_with_parameters: No cached answer, prompted user for trust -> " << a;
}
