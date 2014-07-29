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

#ifndef CORE_TRUST_REMOTE_HELPERS_H_
#define CORE_TRUST_REMOTE_HELPERS_H_

#include <core/trust/tagged_integer.h>
#include <core/trust/visibility.h>

#include <functional>
#include <string>

namespace core
{
namespace trust
{
namespace remote
{
namespace helpers
{
// Functor abstracting pid -> process start time resolving
typedef std::function<std::int64_t(Pid)> ProcessStartTimeResolver;

// Queries the start time of a process by reading /proc/{PID}/stat.
CORE_TRUST_DLL_PUBLIC ProcessStartTimeResolver proc_stat_start_time_resolver();

// Functor abstracting pid -> app name resolving
typedef std::function<std::string(Pid)> AppIdResolver;

// Queries the app armor confinement profile to resolve the application id.
CORE_TRUST_DLL_PUBLIC AppIdResolver aa_get_task_con_app_id_resolver();
}
}
}
}

#endif // CORE_TRUST_REMOTE_HELPERS_H_
