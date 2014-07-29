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

#include <core/trust/remote/helpers.h>

#include <core/posix/process.h>
#include <core/posix/linux/proc/process/stat.h>

#include <sys/apparmor.h>

namespace trust = core::trust;
namespace remote = core::trust::remote;

// Queries the start time of a process by reading /proc/{PID}/stat.
remote::helpers::ProcessStartTimeResolver remote::helpers::proc_stat_start_time_resolver()
{
    return [](trust::Pid pid)
    {
        core::posix::Process process{pid.value};
        core::posix::linux::proc::process::Stat stat;
        process >> stat;

        return stat.start_time;
    };
}

remote::helpers::AppIdResolver remote::helpers::aa_get_task_con_app_id_resolver()
{
    return [](trust::Pid pid)
    {
        static const int app_armor_error{-1};

        // We make sure to clean up the returned string.
        struct Scope
        {
            ~Scope()
            {
                if (con) ::free(con);
            }

            char* con{nullptr};
            char* mode{nullptr};
        } scope;

        // Reach out to apparmor
        auto rc = aa_gettaskcon(pid.value, &scope.con, &scope.mode);

        // From man aa_gettaskcon:
        // On success size of data placed in the buffer is returned, this includes the mode if
        //present and any terminating characters. On error, -1 is returned, and errno(3) is
        //set appropriately.
        if (rc == app_armor_error) throw std::system_error
        {
            errno,
            std::system_category()
        };

        // Safely construct the string
        return std::string
        {
            scope.con ? scope.con : ""
        };
    };
}
