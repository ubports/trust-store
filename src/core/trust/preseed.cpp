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

#include <core/trust/preseed.h>

#include <boost/program_options.hpp>

#include <istream>

namespace Options = boost::program_options;

std::istream& operator>>(std::istream& in, core::trust::Feature& f)
{
    return in >> f.value;
}

std::istream& operator>>(std::istream& in, core::trust::Request::Answer& answer)
{
    std::string s; in >> s;

    if (s == "denied")
        answer = core::trust::Request::Answer::denied;
    else if (s == "granted")
        answer = core::trust::Request::Answer::granted;
    else
        throw std::logic_error{"Could not parse answer: " + s};

    return in;
}

core::trust::Preseed::Configuration core::trust::Preseed::Configuration::parse_from_command_line(int argc, const char** argv)
{
    Options::variables_map vm;

    Options::options_description options{"Known options"};
    options.add_options()
            (Parameters::ForService::name, Options::value<std::string>()->required(), Parameters::ForService::description)
            (Parameters::Request::name, Options::value<std::vector<std::string>>()->composing(), Parameters::Request::description);

    Options::command_line_parser parser
    {
        argc,
        argv
    };

    auto parsed_options = parser.options(options).allow_unregistered().run();
    Options::store(parsed_options, vm);
    Options::notify(vm);

    auto service_name = vm[Parameters::ForService::name].as<std::string>();
    auto requests = vm[Parameters::Request::name].as<std::vector<std::string>>();

    core::trust::Preseed::Configuration config
    {
        core::trust::create_default_store(service_name),
        {} // The empty set.
    };

    for (const auto& request : requests)
    {
        std::stringstream ss{request}; core::trust::Request r;

        // Parse the request.
        ss >> r.from >> r.feature >> r.answer;
        // And set the timestamp to now.
        r.when = std::chrono::system_clock::now();

        config.requests.push_back(r);
    }

    return config;
}

core::posix::exit::Status core::trust::Preseed::main(const core::trust::Preseed::Configuration& configuration)
{
    for (const auto& request : configuration.requests)
        configuration.store->add(request);

    return core::posix::exit::Status::success;
}
