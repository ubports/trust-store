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

#include <core/trust/remote/posix.h>

#include <core/posix/process.h>
#include <core/posix/linux/proc/process/stat.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/format.hpp>

#include <fstream>
#include <functional>
#include <mutex>

namespace trust = core::trust;
namespace remote = core::trust::remote;

remote::posix::Stub::PeerCredentialsResolver remote::posix::Stub::get_sock_opt_credentials_resolver()
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

bool remote::posix::Stub::Session::Registry::has_session_for_uid(core::trust::Uid uid) const
{
    std::lock_guard<std::mutex> lg(guard);
    return sessions.count(uid) > 0;
}

void remote::posix::Stub::Session::Registry::add_session_for_uid(core::trust::Uid uid, Session::Ptr session)
{
    if (not session) throw std::logic_error
    {
        "Cannot add null session to registry."
    };

    std::lock_guard<std::mutex> lg(guard);
    sessions[uid] = session;
}

void remote::posix::Stub::Session::Registry::remove_session_for_uid(core::trust::Uid uid)
{
    std::lock_guard<std::mutex> lg(guard);
    sessions.erase(uid);
}

remote::posix::Stub::Session::Ptr remote::posix::Stub::Session::Registry::resolve_session_for_uid(core::trust::Uid uid)
{
    std::lock_guard<std::mutex> lg(guard);
    return sessions.at(uid);
}

remote::posix::Stub::Session::Session(boost::asio::io_service& io_service)
    : socket{io_service}
{
}

remote::posix::Stub::Ptr remote::posix::Stub::create_stub_for_configuration(const Configuration& config)
{
    remote::posix::Stub::Ptr stub
    {
        new remote::posix::Stub
        {
            config
        }
    };

    stub->start_accept();
    return stub;
}

// Creates a new instance with the given configuration.
// Throws in case of errors.
remote::posix::Stub::Stub(remote::posix::Stub::Configuration configuration)
    : io_service(configuration.io_service),
      end_point{configuration.endpoint},
      acceptor{io_service, end_point},
      start_time_resolver{configuration.start_time_resolver},
      peer_credentials_resolver{configuration.peer_credentials_resolver},
      session_registry{configuration.session_registry}
{
}

void remote::posix::Stub::start_accept()
{
    remote::posix::Stub::Session::Ptr session
    {
        new remote::posix::Stub::Session{io_service}
    };

    acceptor.async_accept(
                session->socket,
                boost::bind(&remote::posix::Stub::on_new_session,
                            shared_from_this(),
                            boost::asio::placeholders::error(),
                            session));
}

remote::posix::Stub::~Stub()
{
    acceptor.cancel();
}

core::trust::Request::Answer remote::posix::Stub::send(
        const core::trust::Agent::RequestParameters& parameters)
{
    // We consider the process start time to prevent from spoofing.
    auto start_time_before_query = start_time_resolver(parameters.application.pid);

    // This call will throw if there is no session known for the uid.
    Session::Ptr session = session_registry->resolve_session_for_uid(parameters.application.uid);

    remote::posix::Request request
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
void remote::posix::Stub::on_new_session(
        const boost::system::error_code& ec,
        const remote::posix::Stub::Session::Ptr& session)
{
    if (ec == boost::asio::error::operation_aborted)
        return;

    if (ec) { start_accept(); return; }

    auto pc = peer_credentials_resolver(session->socket.native_handle());

    session_registry->add_session_for_uid(std::get<0>(pc), session);

    start_accept();
}

void remote::posix::Stub::handle_error_from_socket_operation_for_uid(const boost::system::error_code& ec, trust::Uid uid)
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

bool remote::posix::Stub::has_session_for_uid(trust::Uid uid) const
{
    return session_registry->has_session_for_uid(uid);
}

remote::posix::Skeleton::Ptr remote::posix::Skeleton::create_skeleton_for_configuration(
    const remote::posix::Skeleton::Configuration& configuration)
{
    remote::posix::Skeleton::Ptr skeleton
    {
        new remote::posix::Skeleton
        {
            configuration
        }
    };

    skeleton->start_read();
    return skeleton;
}

remote::posix::Skeleton::Skeleton(const Configuration& configuration)
    : core::trust::remote::Agent::Skeleton{configuration.impl},
      start_time_resolver{configuration.start_time_resolver},
      app_id_resolver{configuration.app_id_resolver},
      description_pattern{configuration.description_format},
      verify_process_start_time{configuration.verify_process_start_time},
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

remote::posix::Skeleton::~Skeleton()
{
    socket.cancel();
}

void remote::posix::Skeleton::start_read()
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

void remote::posix::Skeleton::on_read_finished(const boost::system::error_code& ec, std::size_t size)
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

core::trust::Request::Answer remote::posix::Skeleton::process_incoming_request(const core::trust::remote::posix::Request& request)
{
    if (verify_process_start_time)
    {
        // We first validate the process start time again.
        if (start_time_resolver(request.app_pid) != request.app_start_time) throw std::runtime_error
        {
            "Potential spoofing detected on incoming request."
        };
    }

    auto app_id = app_id_resolver(request.app_pid);

    // And reach out to the user.
    // TODO(tvoss): How to handle exceptions here?

    return authenticate_request_with_parameters(core::trust::Agent::RequestParameters
    {
        request.app_uid,
        request.app_pid,
        app_id,
        request.feature,
        description_pattern
    });
}
