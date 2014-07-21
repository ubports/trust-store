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

#include "core/trust/remote/agent.h"

namespace remote = core::trust::remote;

core::trust::Request::Answer remote::Agent::Stub::prompt_user_for_request(uid_t app_uid, pid_t app_pid, const std::string& app_id, const std::string& description)
{
    return send(app_uid, app_pid, app_id, description);
}

remote::Agent::Skeleton::Skeleton(const std::shared_ptr<core::trust::Agent>& impl) : impl{impl}
{
}

core::trust::Request::Answer remote::Agent::Skeleton::prompt_user_for_request(uid_t app_uid, pid_t app_pid, const std::string& app_id, const std::string& description)
{
    return impl->prompt_user_for_request(app_uid, app_pid, app_id, description);
}
