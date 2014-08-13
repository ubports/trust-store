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

#include <core/trust/daemon.h>

#include <boost/exception/all.hpp>

#include <glog/logging.h>

int main(int argc, const char** argv)
{
    google::InitGoogleLogging("core::trust::Daemon::Skeleton");

    core::trust::Daemon::Skeleton::Configuration configuration;

    try
    {
        configuration = core::trust::Daemon::Skeleton::Configuration::from_command_line(argc, argv);
    } catch(const boost::exception& e)
    {
        std::cerr << "Error during initialization and startup: " << boost::diagnostic_information(e) << std::endl;
        return EXIT_FAILURE;
    }

    return static_cast<int>(core::trust::Daemon::Skeleton::main(configuration));
}
