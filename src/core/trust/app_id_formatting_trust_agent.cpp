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

#include <core/trust/app_id_formatting_trust_agent.h>

#include <regex>

core::trust::AppIdFormattingTrustAgent::AppIdFormattingTrustAgent(const std::shared_ptr<core::trust::Agent>& impl)
    : impl{impl}
{
    if (not impl) throw std::runtime_error
    {
        "Missing agent implementation."
    };
}

// From core::trust::Agent
core::trust::Request::Answer core::trust::AppIdFormattingTrustAgent::authenticate_request_with_parameters(const core::trust::Agent::RequestParameters& incoming_params)
{
    auto params = incoming_params;

    // We post-process the application id and try to extract the unversioned package name.
    // Please see https://wiki.ubuntu.com/AppStore/Interfaces/ApplicationId.
    static const std::regex regex{"(.*)_(.*)"};
    static constexpr std::size_t index_package_app{1};

    // We store our matches here.
    std::smatch match;

    // See if the application id matches the pattern described in
    // https://wiki.ubuntu.com/AppStore/Interfaces/ApplicationId
    if (std::regex_match(params.application.id, match, regex))
    {
        params.application.id = match[index_package_app];
    }

    return impl->authenticate_request_with_parameters(params);
}
