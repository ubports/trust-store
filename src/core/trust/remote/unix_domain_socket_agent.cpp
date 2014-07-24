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

#include <core/trust/remote/unix_domain_socket_agent.h>

#include <core/posix/process.h>
#include <core/posix/linux/proc/process/stat.h>

#include <sys/apparmor.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/format.hpp>

#include <fstream>
#include <functional>
#include <mutex>

namespace trust = core::trust;
namespace remote = core::trust::remote;

// Queries the start time of a process by reading /proc/{PID}/stat.
remote::UnixDomainSocketAgent::ProcessStartTimeResolver remote::UnixDomainSocketAgent::proc_stat_start_time_resolver()
{
    return [](trust::Pid pid)
    {
        core::posix::Process process{pid.value};
        core::posix::linux::proc::process::Stat stat;
        process >> stat;

        return stat.start_time;
    };
}

remote::UnixDomainSocketAgent::Stub::PeerCredentialsResolver remote::UnixDomainSocketAgent::Stub::get_sock_opt_credentials_resolver()
{
    return [](int socket)
    {
        static const int get_sock_opt_error = -1;

        ::ucred peer_credentials { 0, 0, 0 }; ::socklen_t len = sizeof(peer_credentials);

        auto rc = ::getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &peer_credentials, &len);

        if (rc == get_sock_opt_error) throw std::system_error
        {
            errno, std::system_category()
        };

        return std::make_tuple(
                    core::trust::Uid{peer_credentials.uid},
                    core::trust::Pid{peer_credentials.pid},
                    core::trust::Gid{peer_credentials.gid});
    };
}

bool remote::UnixDomainSocketAgent::Stub::Session::Registry::has_session_for_uid(core::trust::Uid uid) const
{
    std::lock_guard<std::mutex> lg(guard);
    return sessions.count(uid) > 0;
}

void remote::UnixDomainSocketAgent::Stub::Session::Registry::add_session_for_uid(core::trust::Uid uid, Session::Ptr session)
{
    if (not session) throw std::logic_error
    {
        "Cannot add null session to registry."
    };

    std::lock_guard<std::mutex> lg(guard);
    sessions[uid] = session;
}

void remote::UnixDomainSocketAgent::Stub::Session::Registry::remove_session_for_uid(core::trust::Uid uid)
{
    std::lock_guard<std::mutex> lg(guard);
    sessions.erase(uid);
}

remote::UnixDomainSocketAgent::Stub::Session::Ptr remote::UnixDomainSocketAgent::Stub::Session::Registry::resolve_session_for_uid(core::trust::Uid uid)
{
    std::lock_guard<std::mutex> lg(guard);
    return sessions.at(uid);
}

remote::UnixDomainSocketAgent::Stub::Session::Session(boost::asio::io_service& io_service)
    : socket{io_service}
{
}

remote::UnixDomainSocketAgent::Stub::Ptr remote::UnixDomainSocketAgent::Stub::create_stub_for_configuration(const Configuration& config)
{
    remote::UnixDomainSocketAgent::Stub::Ptr stub
    {
        new remote::UnixDomainSocketAgent::Stub
        {
            config
        }
    };

    stub->start_accept();
    return stub;
}

// Creates a new instance with the given configuration.
// Throws in case of errors.
remote::UnixDomainSocketAgent::Stub::Stub(remote::UnixDomainSocketAgent::Stub::Configuration configuration)
    : io_service(configuration.io_service),
      end_point{configuration.endpoint},
      acceptor{io_service, end_point},
      start_time_resolver{configuration.start_time_resolver},
      peer_credentials_resolver{configuration.peer_credentials_resolver},
      session_registry{configuration.session_registry}
{
}

void remote::UnixDomainSocketAgent::Stub::start_accept()
{
    remote::UnixDomainSocketAgent::Stub::Session::Ptr session
    {
        new remote::UnixDomainSocketAgent::Stub::Session{io_service}
    };

    acceptor.async_accept(
                session->socket,
                boost::bind(&remote::UnixDomainSocketAgent::Stub::on_new_session,
                            shared_from_this(),
                            boost::asio::placeholders::error(),
                            session));
}

remote::UnixDomainSocketAgent::Stub::~Stub()
{
    acceptor.cancel();
}

core::trust::Request::Answer remote::UnixDomainSocketAgent::Stub::send(
        const core::trust::Agent::RequestParameters& parameters)
{
    // We consider the process start time to prevent from spoofing.
    auto start_time_before_query = start_time_resolver(parameters.application.pid);

    // This call will throw if there is no session known for the uid.
    Session::Ptr session = session_registry->resolve_session_for_uid(parameters.application.uid);

    remote::UnixDomainSocketAgent::Request request
    {
        parameters.application.uid,
        parameters.application.pid,
        parameters.feature,
        start_time_before_query
    };

    boost::system::error_code ec;

    boost::asio::write(
                session->socket,
                boost::asio::buffer(&request, sizeof(request)),
                ec);

    if (ec)
        handle_error_from_socket_operation_for_uid(ec, parameters.application.uid);

    core::trust::Request::Answer answer
    {
        core::trust::Request::Answer::denied
    };

    boost::asio::read(
                session->socket,
                boost::asio::buffer(&answer, sizeof(answer)),
                ec);

    if (ec)
        handle_error_from_socket_operation_for_uid(ec, parameters.application.uid);

    // And finally, we check on the process start time.
    auto start_time_after_query = start_time_resolver(parameters.application.pid);

    // We consider the process start time to prevent from spoofing. That is,
    // if the process start times differ here, we would authenticate a different process and
    // with that potentially a different app.
    if (start_time_before_query != start_time_after_query) throw std::runtime_error
    {
        "Detected a spoofing attempt, process start times before"
        "and after authentication do not match."
    };

    // We will only ever return if we encountered no errors during communication
    // with the other side.
    return answer;
}

// Called in case of an incoming connection.
void remote::UnixDomainSocketAgent::Stub::on_new_session(
        const boost::system::error_code& ec,
        const remote::UnixDomainSocketAgent::Stub::Session::Ptr& session)
{
    if (ec == boost::asio::error::operation_aborted)
        return;

    if (ec) { start_accept(); return; }

    auto pc = peer_credentials_resolver(session->socket.native_handle());

    session_registry->add_session_for_uid(std::get<0>(pc), session);

    start_accept();
}

void remote::UnixDomainSocketAgent::Stub::handle_error_from_socket_operation_for_uid(const boost::system::error_code& ec, trust::Uid uid)
{
    switch (ec.value())
    {
    // We treat all the following errors as indications for the peer having
    // gone away. For that, we update the Session::Registry
    case boost::asio::error::basic_errors::access_denied:
    case boost::asio::error::basic_errors::broken_pipe:
    case boost::asio::error::basic_errors::connection_aborted:
    case boost::asio::error::basic_errors::connection_refused:
    case boost::asio::error::basic_errors::connection_reset:
        session_registry->remove_session_for_uid(uid);
        break;
    default:
        break;
    }

    // By default, we just rethrow.
    throw std::system_error{ec.value(), std::system_category()};
}

bool remote::UnixDomainSocketAgent::Stub::has_session_for_uid(trust::Uid uid) const
{
    return session_registry->has_session_for_uid(uid);
}

remote::UnixDomainSocketAgent::Skeleton::AppIdResolver remote::UnixDomainSocketAgent::Skeleton::aa_get_task_con_app_id_resolver()
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

remote::UnixDomainSocketAgent::Skeleton::Ptr remote::UnixDomainSocketAgent::Skeleton::create_skeleton_for_configuration(
    const remote::UnixDomainSocketAgent::Skeleton::Configuration& configuration)
{
    remote::UnixDomainSocketAgent::Skeleton::Ptr skeleton
    {
        new remote::UnixDomainSocketAgent::Skeleton
        {
            configuration
        }
    };

    skeleton->start_read();
    return skeleton;
}

remote::UnixDomainSocketAgent::Skeleton::Skeleton(const Configuration& configuration)
    : core::trust::remote::Agent::Skeleton{configuration.impl},
      start_time_resolver{configuration.start_time_resolver},
      app_id_resolver{configuration.app_id_resolver},
      description_pattern{configuration.description_format},
      endpoint{configuration.endpoint},
      socket{configuration.io_service}
{
    try
    {
        socket.connect(configuration.endpoint);
    } catch(const boost::exception& e)
    {
        throw std::runtime_error
        {
            "Could not connect to endpoint: " + endpoint.path()
        };
    }
}

remote::UnixDomainSocketAgent::Skeleton::~Skeleton()
{
    socket.cancel();
}

void remote::UnixDomainSocketAgent::Skeleton::start_read()
{
    Ptr sp{shared_from_this()};

    boost::asio::async_read(
                socket,
                boost::asio::buffer(&request, sizeof(request)),
                [sp](const boost::system::error_code& ec, std::size_t size)
                {
                    sp->on_read_finished(ec, size);
                });
}

void remote::UnixDomainSocketAgent::Skeleton::on_read_finished(const boost::system::error_code& ec, std::size_t size)
{
    if (ec == boost::asio::error::operation_aborted)
        return;

    if (ec) { start_read(); return; }

    if (size != sizeof(request))
        return;

    auto answer = process_incoming_request(request);

    // We send back our answer, we might throw here and just let the
    // exception propagate through the stack.
    boost::asio::write(socket, boost::asio::buffer(&answer, sizeof(answer)));
    // And restart reading.
    start_read();
}

core::trust::Request::Answer remote::UnixDomainSocketAgent::Skeleton::process_incoming_request(const core::trust::remote::UnixDomainSocketAgent::Request& request)
{
    // We first validate the process start time again.
    if (start_time_resolver(request.app_pid) != request.app_start_time) throw std::runtime_error
    {
        "Potential spoofing detected on incoming request."
    };

    // Assemble the description.
    auto app_id = app_id_resolver(request.app_pid);
    auto description = (boost::format{description_pattern} % app_id).str();

    // And reach out to the user.
    // TODO(tvoss): How to handle exceptions here?

    return authenticate_request_with_parameters(core::trust::Agent::RequestParameters
    {
        request.app_uid,
        request.app_pid,
        app_id,
        request.feature,
        description
    });
}
