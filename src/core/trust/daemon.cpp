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

#include <core/trust/cached_agent.h>
#include <core/trust/expose.h>
#include <core/trust/store.h>

#include <core/trust/mir_agent.h>

#include <mirclient/mir_toolkit/mir_client_library.h>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

#include <thread>

namespace Options = boost::program_options;

namespace
{
    struct Runtime
    {
        // Do not execute in parallel, serialize
        // accesses.
        static constexpr std::size_t concurrency_hint{1};

        // Our evil singleton pattern. Not bad though, we control the
        // entire executable and rely on automatic cleanup of static
        // instances.
        static Runtime& instance()
        {
            static Runtime runtime;
            return runtime;
        }

        // Our io_service instance exposed to remote agents.
        boost::asio::io_service io_service
        {
            concurrency_hint
        };
        // We keep the io_service alive and introduce some artifical
        // work.
        boost::asio::io_service::work keep_alive
        {
            io_service
        };
    };
}

const std::map<std::string, core::trust::Daemon::LocalAgentFactory>& core::trust::Daemon::known_local_agent_factories()
{
    static std::map<std::string, LocalAgentFactory> lut
    {
        {
            "MirAgent",
            [](const std::string& service_name, const Dictionary& dict)
            {
                if (dict.count("trusted-mir-socket") == 0) throw std::runtime_error
                {
                    "Missing endpoint specification for accessing Mir's trusted socket."
                };

                auto trusted_mir_socket = dict.at("trusted-mir-socket");
                auto mir_connection = mir_connect_sync(trusted_mir_socket.c_str(), service_name.c_str());
                auto agent = core::trust::mir::create_agent_for_mir_connection(mir_connection);

                return agent;
            }
        }
    };
    return lut;
}

const std::map<std::string, core::trust::Daemon::RemoteAgentFactory>& core::trust::Daemon::known_remote_agent_factories()
{
    static std::map<std::string, RemoteAgentFactory> lut
    {
        {
            "UnixDomainSocketRemoteAgent",
            [](const std::string& service_name, const std::shared_ptr<Agent>& agent, const Dictionary& dict)
            {
                if (dict.count("endpoint") == 0) throw std::runtime_error
                {
                    "Missing endpoint specification for UnixDomainSocketRemoteAgent."
                };

                core::trust::remote::UnixDomainSocketAgent::Skeleton::Configuration config
                {
                    agent,
                    Runtime::instance().io_service,
                    boost::asio::local::stream_protocol::endpoint{dict.at("endpoint")},
                    core::trust::remote::UnixDomainSocketAgent::proc_stat_start_time_resolver(),
                    core::trust::remote::UnixDomainSocketAgent::Skeleton::aa_get_task_con_app_id_resolver(),
                    dict.count("description-pattern") > 0 ?
                            dict.at("description-pattern") :
                            "Application %1% is trying to access " + service_name + "."
                };

                return core::trust::remote::UnixDomainSocketAgent::Skeleton::create_skeleton_for_configuration(config);
            }
        }
    };
    return lut;
}

// Parses the configuration from the given command line.
core::trust::Daemon::Configuration core::trust::Daemon::Configuration::parse_from_command_line(int argc, char** argv)
{
    Options::variables_map vm;

    Options::options_description options{"Known options"};
    options.add_options()
            (Parameters::ForService::name, Options::value<std::string>()->required(), Parameters::ForService::description)
            (Parameters::LocalAgent::name, Options::value<std::string>()->required(), Parameters::LocalAgent::description)
            (Parameters::RemoteAgent::name, Options::value<std::string>()->required(), Parameters::RemoteAgent::description);

    Options::command_line_parser parser
    {
        argc,
        argv
    };

    std::vector<std::string> unrecognized;

    try
    {
        auto parsed_options = parser.options(options).allow_unregistered().run();
        Options::store(parsed_options, vm);
        Options::notify(vm);

        unrecognized = Options::collect_unrecognized(
                    parsed_options.options,
                    Options::exclude_positional);

    } catch(const boost::exception& e)
    {
        throw std::runtime_error
        {
            "Error parsing command line: " + boost::diagnostic_information(e)
        };
    }

    core::trust::Daemon::Dictionary dict;

    for (std::string& element : unrecognized)
    {
        if (element.find("--") != 0)
            continue;

        auto idx = element.find("=");

        if (idx == std::string::npos)
            continue;

        auto key = element.substr(0, idx).substr(2, std::string::npos);
        auto value = element.substr(idx+1, std::string::npos);

        dict[key] = value;
    }

    auto service_name = vm[Parameters::ForService::name].as<std::string>();

    auto local_agent_factory = core::trust::Daemon::known_local_agent_factories()
            .at(vm[Parameters::LocalAgent::name]
            .as<std::string>());

    auto remote_agent_factory = core::trust::Daemon::known_remote_agent_factories()
            .at(vm[Parameters::RemoteAgent::name]
            .as<std::string>());

    auto local_store = core::trust::create_default_store(service_name);
    auto local_agent = local_agent_factory(service_name, dict);

    auto cached_agent = std::make_shared<core::trust::CachedAgent>(
        core::trust::CachedAgent::Configuration
        {
            local_agent,
            local_store
        });

    auto remote_agent = remote_agent_factory(service_name, cached_agent, dict);

    return core::trust::Daemon::Configuration
    {
        service_name,
        {local_store, local_agent},
        {remote_agent}
    };
}

// Executes the daemon with the given configuration.
core::posix::exit::Status core::trust::Daemon::main(const core::trust::Daemon::Configuration& configuration)
{
    auto trap = core::posix::trap_signals_for_all_subsequent_threads(
    {
        core::posix::Signal::sig_term
    });

    trap->signal_raised().connect([trap](core::posix::Signal)
    {
        trap->stop();
    });

    // Expose the local store to the bus, keeping it exposed for the
    // lifetime of the returned token.
    auto token = core::trust::expose_store_to_session_with_name(
                configuration.local.store,
                configuration.service_name);

    // And start executing the io_service.
    std::thread worker
    {
        []() { Runtime::instance().io_service.run(); }
    };

    trap->run();

    Runtime::instance().io_service.stop();

    if (worker.joinable())
        worker.join();

    return core::posix::exit::Status::success;
}

int main(int argc, char** argv)
{    
    core::trust::Daemon::Configuration configuration;

    try
    {
        configuration = core::trust::Daemon::Configuration::parse_from_command_line(argc, argv);
    } catch(const boost::exception& e)
    {
        std::cerr << "Error during initialization and startup: " << boost::diagnostic_information(e) << std::endl;
    }

    return static_cast<int>(core::trust::Daemon::main(configuration));
}
