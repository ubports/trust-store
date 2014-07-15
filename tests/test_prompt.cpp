/*
 * Copyright © 2013 Canonical Ltd.
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

#include <core/trust/mir/prompt_main.h>

#include <boost/program_options.hpp>

int main(int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
        std::cout << i << ": " << argv[i] << std::endl;

    boost::program_options::options_description options;
    options.add_options()
            (core::trust::mir::cli::option_server_socket, boost::program_options::value<std::string>(), "Mir server socket to connect to.")
            (core::trust::mir::cli::option_description, boost::program_options::value<std::string>(), "Extended description of the prompt.")
            (core::trust::mir::cli::option_title, boost::program_options::value<std::string>(), "Title of the prompt");

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), vm);
    boost::program_options::notify(vm);

    // The server-socket option is mandatory.
    if (vm.count(core::trust::mir::cli::option_server_socket) == 0)
        ::abort();
    // We expect the argument to be in the format fd://%d.
    if (vm[core::trust::mir::cli::option_server_socket].as<std::string>().find("fd://") != 0)
        ::abort();
    // The title option is mandatory.
    if (vm.count(core::trust::mir::cli::option_title) == 0)
        ::abort();

    return EXIT_SUCCESS;
}
