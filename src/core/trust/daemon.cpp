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

#include <core/trust/app_id_formatting_trust_agent.h>
#include <core/trust/cached_agent.h>
#include <core/trust/expose.h>
#include <core/trust/i18n.h>
#include <core/trust/store.h>
#include <core/trust/white_listing_agent.h>

#include <core/trust/mir_agent.h>

#include <core/trust/remote/agent.h>
#include <core/trust/remote/dbus.h>
#include <core/trust/remote/helpers.h>
#include <core/trust/remote/posix.h>

#include <core/trust/terminal_agent.h>

#include <core/trust/cached_agent_glog_reporter.h>

#include <core/dbus/asio/executor.h>

#include <boost/asio.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

#include <thread>
#include <chrono>

namespace Options = boost::program_options;

namespace
{
    struct Runtime
    {
        // Do not execute in parallel, serialize
        // accesses.
        static constexpr std::size_t concurrency_hint{2};

        // Our evil singleton pattern. Not bad though, we control the
        // entire executable and rely on automatic cleanup of static
        // instances.
        static Runtime& instance()
        {
            static Runtime runtime;
            return runtime;
        }

        ~Runtime()
        {
            io_service.stop();

            if (worker1.joinable())
                worker1.join();

            if (worker2.joinable())
                worker2.join();
        }

        // We trap sig term to ensure a clean shutdown.
        std::shared_ptr<core::posix::SignalTrap> signal_trap
        {
            core::posix::trap_signals_for_all_subsequent_threads(
            {
                core::posix::Signal::sig_term,
                core::posix::Signal::sig_int
            })
        };

        // Our io_service instance exposed to remote agents.
        boost::asio::io_service io_service
        {
            concurrency_hint
        };

        // We keep the io_service alive and introduce some artificial
        // work.
        boost::asio::io_service::work keep_alive
        {
            io_service
        };

        // We immediate execute the io_service instance
        std::thread worker1
        {
            std::thread{[this]() { io_service.run(); }}
        };

        std::thread worker2
        {
            std::thread{[this]() { io_service.run(); }}
        };
    };

    core::trust::Daemon::Dictionary fill_dictionary_from_unrecognized_options(const Options::parsed_options& parsed_options)
    {
        auto unrecognized = Options::collect_unrecognized(
                    parsed_options.options,
                    Options::exclude_positional);

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

        return dict;
    }

    struct DummyAgent : public core::trust::Agent
    {
        DummyAgent(core::trust::Request::Answer canned_answer)
            : canned_answer{canned_answer}
        {
        }

        core::trust::Request::Answer authenticate_request_with_parameters(const RequestParameters&) override
        {
            return canned_answer;
        }

        core::trust::Request::Answer canned_answer;
    };

    core::dbus::Bus::Ptr bus_from_name(const std::string& bus_name)
    {
        core::dbus::Bus::Ptr bus;

        if (bus_name == "system")
            bus = std::make_shared<core::dbus::Bus>(core::dbus::WellKnownBus::system);
        else if (bus_name == "session")
            bus = std::make_shared<core::dbus::Bus>(core::dbus::WellKnownBus::session);
        else if (bus_name == "system_with_address_from_env")
            bus = std::make_shared<core::dbus::Bus>(core::posix::this_process::env::get_or_throw("DBUS_SYSTEM_BUS_ADDRESS"));
        else if (bus_name == "session_with_address_from_env")
            bus = std::make_shared<core::dbus::Bus>(core::posix::this_process::env::get_or_throw("DBUS_SESSION_BUS_ADDRESS"));

        if (not bus) throw std::runtime_error
        {
            "Could not create bus for name: " + bus_name
        };

        bus->install_executor(core::dbus::asio::make_executor(bus, Runtime::instance().io_service));

        return bus;
    }

    core::dbus::Bus::Ptr bus_from_dictionary(const core::trust::Daemon::Dictionary& dict)
    {
        return bus_from_name(dict.at("bus"));
    }
}

const std::map<std::string, core::trust::Daemon::Skeleton::LocalAgentFactory>& core::trust::Daemon::Skeleton::known_local_agent_factories()
{
    static std::map<std::string, LocalAgentFactory> lut
    {
        {
            std::string{Daemon::LocalAgents::MirAgent::name},
            [](const std::string& service_name, const Dictionary& dict)
            {
                if (dict.count("trusted-mir-socket") == 0) throw std::runtime_error
                {
                    "Missing endpoint specification for accessing Mir's trusted socket."
                };

                auto trusted_mir_socket = dict.at("trusted-mir-socket");

                // TODO: log reconnection attempts
                int connection_attempts = 5;
                while (true)
                {
                    try
                    {
                        return core::trust::mir::create_agent_for_mir_connection(
                                    core::trust::mir::connect(
                                        trusted_mir_socket,
                                        service_name));
                    }
                    catch (core::trust::mir::InvalidMirConnection const&)
                    {
                        if (--connection_attempts == 0)
                            throw;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::cerr << "reattempt connection to mir..."<< std::endl;
                }
            }
        },
        {
            std::string{Daemon::LocalAgents::TerminalAgent::name},
            [](const std::string& service_name, const Dictionary&)
            {
                return std::make_shared<core::trust::TerminalAgent>(service_name);
            }
        },
        {
            "TheAlwaysDenyingLocalAgent",
            [](const std::string&, const Dictionary&)
            {
                auto agent = std::shared_ptr<core::trust::Agent>
                {
                    new DummyAgent{core::trust::Request::Answer::denied}
                };

                return agent;
            }
        }
    };
    return lut;
}

const std::map<std::string, core::trust::Daemon::Skeleton::RemoteAgentFactory>& core::trust::Daemon::Skeleton::known_remote_agent_factories()
{
    static std::map<std::string, RemoteAgentFactory> lut
    {
        {
            std::string{Daemon::RemoteAgents::UnixDomainSocketRemoteAgent::name},
            [](const std::string& service_name, const std::shared_ptr<Agent>& agent, const Dictionary& dict)
            {
                if (dict.count("endpoint") == 0) throw std::runtime_error
                {
                    "Missing endpoint specification for UnixDomainSocketRemoteAgent."
                };

                core::trust::remote::posix::Skeleton::Configuration config
                {
                    agent,
                    Runtime::instance().io_service,
                    boost::asio::local::stream_protocol::endpoint{dict.at("endpoint")},
                    core::trust::remote::helpers::proc_stat_start_time_resolver(),
                    core::trust::remote::helpers::aa_get_task_con_app_id_resolver(),
                    dict.count("description-pattern") > 0 ?
                            dict.at("description-pattern") :
                            "Application %1% is trying to access " + service_name + "."
                };

                return core::trust::remote::posix::Skeleton::create_skeleton_for_configuration(config);
            }
        },
        {
            std::string{Daemon::RemoteAgents::DBusRemoteAgent::name},
            [](const std::string& service_name, const std::shared_ptr<Agent>& agent, const Dictionary& dict)
            {
                if (dict.count("bus") == 0) throw std::runtime_error
                {
                    "Missing bus specifier, please choose from {system, session}."
                };

                auto bus = bus_from_dictionary(dict);

                std::string dbus_service_name = core::trust::remote::dbus::default_service_name_prefix + std::string{"."} + service_name;

                auto service = core::dbus::Service::use_service(bus, dbus_service_name);
                auto object = service->object_for_path(
                            core::dbus::types::ObjectPath
                            {
                                core::trust::remote::dbus::default_agent_registry_path
                            });

                core::trust::remote::dbus::Agent::Skeleton::Configuration config
                {
                    agent,
                    object,
                    service,
                    bus,
                    core::trust::remote::helpers::aa_get_task_con_app_id_resolver()
                };

                return std::make_shared<core::trust::remote::dbus::Agent::Skeleton>(config);
            }
        }
    };
    return lut;
}

// Parses the configuration from the given command line.
core::trust::Daemon::Skeleton::Configuration core::trust::Daemon::Skeleton::Configuration::from_command_line(int argc, const char** argv)
{
    Options::variables_map vm;
    Dictionary dict;

    Options::options_description options{"Known options"};
    options.add_options()
            (Parameters::ForService::name, Options::value<std::string>()->required(), Parameters::ForService::description)
            (Parameters::WithTextDomain::name, Options::value<std::string>(), Parameters::WithTextDomain::description)
            (Parameters::StoreBus::name, Options::value<std::string>()->default_value("session"), Parameters::StoreBus::description)
            (Parameters::LocalAgent::name, Options::value<std::string>()->required(), Parameters::LocalAgent::description)
            (Parameters::RemoteAgent::name, Options::value<std::string>()->required(), Parameters::RemoteAgent::description);

    Options::command_line_parser parser
    {
        argc,
        argv
    };

    try
    {
        auto parsed_options = parser.options(options).allow_unregistered().run();
        Options::store(parsed_options, vm);
        Options::notify(vm);

        dict = fill_dictionary_from_unrecognized_options(parsed_options);
    } catch(const boost::exception& e)
    {
        throw std::runtime_error
        {
            "Error parsing command line: " + boost::diagnostic_information(e)
        };
    }

    auto service_name = vm[Parameters::ForService::name].as<std::string>();

    auto service_text_domain = service_name;

    if (vm.count(Parameters::WithTextDomain::name) > 0)
        service_text_domain = vm[Parameters::WithTextDomain::name].as<std::string>();

    core::trust::i18n::set_service_text_domain(service_text_domain);

    auto local_agent_factory = core::trust::Daemon::Skeleton::known_local_agent_factories()
            .at(vm[Parameters::LocalAgent::name].as<std::string>());

    auto remote_agent_factory = core::trust::Daemon::Skeleton::known_remote_agent_factories()
            .at(vm[Parameters::RemoteAgent::name].as<std::string>());

    auto local_store = core::trust::create_default_store(service_name);
    auto local_agent = local_agent_factory(service_name, dict);

    auto cached_agent = std::make_shared<core::trust::CachedAgent>(
        core::trust::CachedAgent::Configuration
        {
            local_agent,
            local_store,
            std::make_shared<core::trust::CachedAgentGlogReporter>(
                    core::trust::CachedAgentGlogReporter::Configuration{})
        });

    auto whitelisting_agent = std::make_shared<core::trust::WhiteListingAgent>([](const core::trust::Agent::RequestParameters& params) -> bool
    {
        static auto unconfined_predicate = core::trust::WhiteListingAgent::always_grant_for_unconfined();
        return unconfined_predicate(params) || params.application.id == "com.ubuntu.camera_camera";
    }, cached_agent);

    auto formatting_agent = std::make_shared<core::trust::AppIdFormattingTrustAgent>(whitelisting_agent);

    auto remote_agent = remote_agent_factory(service_name, formatting_agent, dict);

    return core::trust::Daemon::Skeleton::Configuration
    {
        service_name,
        bus_from_name(vm[Parameters::StoreBus::name].as<std::string>()),
        {local_store, formatting_agent},
        {remote_agent}
    };
}

// Executes the daemon with the given configuration.
core::posix::exit::Status core::trust::Daemon::Skeleton::main(const core::trust::Daemon::Skeleton::Configuration& configuration)
{
    Runtime::instance().signal_trap->signal_raised().connect([](core::posix::Signal)
    {
        Runtime::instance().signal_trap->stop();
    });

    std::thread worker
    {
        [configuration]() { configuration.bus->run(); }
    };

    // Expose the local store to the bus, keeping it exposed for the
    // lifetime of the returned token.
    auto token = core::trust::expose_store_to_bus_with_name(
                configuration.local.store,
                configuration.bus,
                configuration.service_name);

    Runtime::instance().signal_trap->run();

    configuration.bus->stop();

    if (worker.joinable())
        worker.join();

    return core::posix::exit::Status::success;
}

const std::map<std::string, core::trust::Daemon::Stub::RemoteAgentFactory>& core::trust::Daemon::Stub::known_remote_agent_factories()
{
    static std::map<std::string, RemoteAgentFactory> lut
    {
        {
            std::string{Daemon::RemoteAgents::UnixDomainSocketRemoteAgent::name},
            [](const std::string&, const Dictionary& dict)
            {
                if (dict.count("endpoint") == 0) throw std::runtime_error
                {
                    "Missing endpoint specification for UnixDomainSocketRemoteAgent."
                };

                core::trust::remote::posix::Stub::Configuration config
                {
                    Runtime::instance().io_service,
                    boost::asio::local::stream_protocol::endpoint{dict.at("endpoint")},
                    core::trust::remote::helpers::proc_stat_start_time_resolver(),
                    core::trust::remote::posix::Stub::get_sock_opt_credentials_resolver(),
                    std::make_shared<core::trust::remote::posix::Stub::Session::Registry>()
                };

                return core::trust::remote::posix::Stub::create_stub_for_configuration(config);
            }
        },
        {
            std::string{Daemon::RemoteAgents::DBusRemoteAgent::name},
            [](const std::string& service_name, const Dictionary& dict)
            {
                auto bus = bus_from_dictionary(dict);

                std::string dbus_service_name = core::trust::remote::dbus::default_service_name_prefix + std::string{"."} + service_name;

                auto service = core::dbus::Service::add_service(bus, dbus_service_name);
                auto object = service->add_object_for_path(
                            core::dbus::types::ObjectPath
                            {
                                core::trust::remote::dbus::default_agent_registry_path
                            });

                core::trust::remote::dbus::Agent::Stub::Configuration config
                {
                    object,
                    bus
                };

                return std::make_shared<core::trust::remote::dbus::Agent::Stub>(config);
            }
        }
    };

    return lut;
}

core::trust::Daemon::Stub::Configuration core::trust::Daemon::Stub::Configuration::from_command_line(int argc, const char** argv)
{
    Options::variables_map vm;
    Dictionary dict;

    Options::options_description options{"Known options"};
    options.add_options()
            (Parameters::ForService::name, Options::value<std::string>()->required(), Parameters::ForService::description)
            (Parameters::RemoteAgent::name, Options::value<std::string>()->required(), Parameters::RemoteAgent::description);

    Options::command_line_parser parser
    {
        argc,
        argv
    };

    try
    {
        auto parsed_options = parser.options(options).allow_unregistered().run();
        Options::store(parsed_options, vm);
        Options::notify(vm);

        dict = fill_dictionary_from_unrecognized_options(parsed_options);
    } catch(const boost::exception& e)
    {
        throw std::runtime_error
        {
            "Error parsing command line: " + boost::diagnostic_information(e)
        };
    }

    auto service_name = vm[Parameters::ForService::name].as<std::string>();

    auto remote_agent_factory = core::trust::Daemon::Stub::known_remote_agent_factories()
            .at(vm[Parameters::RemoteAgent::name].as<std::string>());

    auto remote_agent = remote_agent_factory(service_name, dict);

    return core::trust::Daemon::Stub::Configuration
    {
        service_name,
        remote_agent
    };
}

namespace
{
// A very simple class to help with testing.
// A user can feed a request to the stub.
struct Shell : public std::enable_shared_from_this<Shell>
{
    Shell(const std::shared_ptr<core::trust::Agent>& agent)
        : agent{agent},
          stdin{Runtime::instance().io_service, STDIN_FILENO},
          app_id_resolver{core::trust::remote::helpers::aa_get_task_con_app_id_resolver()}
    {
    }

    // Prints out the initial prompt and initiates a read operation on stdin.
    void start()
    {
        std::cout << "This is the super simple, interactive shell of the trust::store Daemon" << std::endl;
        std::cout << "The following commands are known:" << std::endl;
        std::cout << "  Enter a line like 'pid uid feature' to issue a query with the given parameters." << std::endl;

        start_read();
    }

    // Stops any outstanding read operation from stdin.
    void stop()
    {
        stdin.cancel();
    }

    // Prints the shell prompt and starts an asynchronous read on stdin.
    void start_read()
    {
        // Our shell prompt.
        std::cout << "> " << std::flush;

        // The async read operation
        boost::asio::async_read_until(
                    stdin,  // From stdin
                    buffer, // Into a buffer
                    '\n',   // Until we get a newline
                    boost::bind(
                        &Shell::read_finished,
                        shared_from_this(),
                        boost::asio::placeholders::error(),
                        boost::asio::placeholders::bytes_transferred()));
    }

    // Invoked in case of errors or if one line has been read from stdin.
    void read_finished(const boost::system::error_code& ec, std::size_t)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (ec)
        {
            start_read();
            return;
        }

        core::trust::Agent::RequestParameters params;
        std::istream ss{&buffer};
        ss >> params.application.pid.value >> params.application.uid.value >> params.feature.value;

        // We fix up the parameters.
        params.application.id = app_id_resolver(params.application.pid);

        std::cout << agent->authenticate_request_with_parameters(params) << std::endl;

        buffer.consume(buffer.size());

        start_read();
    }

    // The agent we query for incoming requests.
    std::shared_ptr<core::trust::Agent> agent;
    // Async descriptor for reading from stdin.
    boost::asio::posix::stream_descriptor stdin;
    // The buffer we read in.
    boost::asio::streambuf buffer;
    // We use some helpers to fix up the quite limited requests we parse from the
    // command line
    core::trust::remote::helpers::AppIdResolver app_id_resolver;
};
}

// Executes the daemon with the given configuration.
core::posix::exit::Status core::trust::Daemon::Stub::main(const core::trust::Daemon::Stub::Configuration& configuration)
{
    // We setup our minimal shell here.
    auto shell = std::make_shared<Shell>(configuration.remote.agent);

    Runtime::instance().signal_trap->signal_raised().connect([](core::posix::Signal)
    {
        Runtime::instance().signal_trap->stop();
    });

    // We start up our shell
    shell->start();

    // Wait until signal arrives.
    Runtime::instance().signal_trap->run();

    shell->stop();

    return core::posix::exit::Status::success;
}


