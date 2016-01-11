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
/** @brief Name of the environment variable that triggers loading of the testability plugin. */
static constexpr const char* option_testability
{
    "QT_LOAD_TESTABILITY"
};
}
namespace cli
{
/** @brief Mir server socket to connect to. */
static constexpr const char* option_server_socket
{
    "mir_server_socket"
};

/** @brief Icon of the requesting app. */
static constexpr const char* option_icon
{
    "icon"
};

/** @brief Name of the requesting app. */
static constexpr const char* option_name
{
    "name"
};

/** @brief Id of the requesting app. */
static constexpr const char* option_id
{
    "id"
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

/** @brief Loads the Qt Testability plugin if provided. */
static constexpr const char* option_testability
{
    "testability"
};
}
}
}
}

#endif // CORE_TRUST_MIR_PROMPT_MAIN_H_
