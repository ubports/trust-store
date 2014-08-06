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

#ifndef CORE_TRUST_MIR_PROMPT_MAIN_H_
#define CORE_TRUST_MIR_PROMPT_MAIN_H_

namespace core
{
namespace trust
{
namespace mir
{
namespace env
{
/** @brief Name of the environment variable to pass Mir's endpoint in. */
static constexpr const char* option_mir_socket
{
    "MIR_SOCKET"
};
}
namespace cli
{
/** @brief Mir server socket to connect to. */
static constexpr const char* option_server_socket
{
    "mir_server_socket"
};

/** @brief Title of the prompt. */
static constexpr const char* option_title
{
    "title"
};

/** @brief Extended description of the prompt. */
static constexpr const char* option_description
{
    "description"
};

/** @brief Only checks command-line parameters and does not execute any actions. */
static constexpr const char* option_testing
{
    "testing"
};
}
}
}
}

#endif // CORE_TRUST_MIR_PROMPT_MAIN_H_
