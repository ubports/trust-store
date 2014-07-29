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

#ifndef CORE_TRUST_REMOTE_UNIX_DOMAIN_SOCKET_AGENT_H_
#define CORE_TRUST_REMOTE_UNIX_DOMAIN_SOCKET_AGENT_H_

#include <core/trust/remote/agent.h>
#include <core/trust/remote/helpers.h>

#include <core/posix/process.h>
#include <core/posix/linux/proc/process/stat.h>

#include <sys/apparmor.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <fstream>
#include <functional>
#include <mutex>

namespace core
{
namespace trust
{
namespace remote
{
// A remote agent implementation that exposes a unix domain socket in the
// filesystem. The stub implementation exposes the socket and handles incoming
// connections from its skeletons. For incoming requests, the stub selects the
// handling skeleton based on the user id associated with the request.
namespace posix
{
// Our distilled-down request that we share between stub and skeleton.
struct CORE_TRUST_DLL_PUBLIC Request
{
    // Id of the user that the requesting app is running under.
    Uid app_uid;
    // The process id of the requesting app.
    Pid app_pid;
    // The service-specific feature.
    Feature feature;
    // We want to prevent from spoofing and send over the process start time.
    // In seconds since the epoch.
    std::int64_t app_start_time;
};

// Models the sending end of a remote agent, meant to be used by trusted helpers.
class CORE_TRUST_DLL_PUBLIC Stub
        : public core::trust::remote::Agent::Stub,
        public std::enable_shared_from_this<Stub>
{
public:
    // Just for convenience
    typedef std::shared_ptr<Stub> Ptr;

    // Functor resolving a socket's peer's credentials
    typedef std::function<
        std::tuple<
            Uid,  // The user ID of the peer
            Pid,  // The process ID of the peer
            Gid   // The group ID of the peer
        >(int)      // Handle to the socket
    > PeerCredentialsResolver;

    // Returns a peer credentials resolver that leverages getsockopt.
    static PeerCredentialsResolver get_sock_opt_credentials_resolver();

    // We create a session per incoming connection.
    struct Session
    {
        // Just for convenience
        typedef std::shared_ptr<Session> Ptr;

        // A fancy map implementing the monitor pattern.
        // All functions in this class are thread-safe, but _not_ reentrant.
        class Registry
        {
        public:
            typedef std::shared_ptr<Registry> Ptr;

            Registry() = default;
            virtual ~Registry() = default;

            // Returns true iff the registry instance contains a session for the given user id.
            virtual bool has_session_for_uid(Uid uid) const;

            // Adds the given session for the given uid to the registry.
            virtual void add_session_for_uid(Uid uid, Session::Ptr session);

            // Removes the session instance for the given user id.
            virtual void remove_session_for_uid(Uid uid);

            // Queries the session for the given user id.
            // Throws std::out_of_range if no session is known for the user id.
            virtual Session::Ptr resolve_session_for_uid(Uid uid);

        private:
            mutable std::mutex guard;
            std::map<core::trust::Uid, Session::Ptr> sessions;
        };

        // Creates a new session.
        Session(boost::asio::io_service& io_service);

        // The socket we are operating on.
        boost::asio::local::stream_protocol::socket socket;
        // We have to synchronize requests per session.
        std::mutex lock;
    };

    // All creation time arguments go here.
    struct Configuration
    {
        // The runtime instance to associate to.
        boost::asio::io_service& io_service;
        // The endpoint in the filesystem.
        boost::asio::local::stream_protocol::endpoint endpoint;
        // Helper to map pid -> process start time
        helpers::ProcessStartTimeResolver start_time_resolver;
        // Helper to resolve peer credentials
        PeerCredentialsResolver peer_credentials_resolver;
        // A synchronized registry of all known sessions.
        Session::Registry::Ptr session_registry;
    };

    // Creates a stub instance for the given configuration.
    // The returned instance is waiting for incoming connections.
    static Ptr create_stub_for_configuration(const Configuration& config);

    // Frees all resources and cancels any outstanding async operation
    // on the endpoint.
    virtual ~Stub();

    // From core::trust::remote::Agent::Stub.
    core::trust::Request::Answer send(const core::trust::Agent::RequestParameters& parameters) override;

    // For testing purposes
    bool has_session_for_uid(Uid uid) const;

private:
    // Creates a new instance with the given configuration.
    // Throws in case of errors.
    Stub(Configuration configuration);

    // Starts listening on the endpoing configured at creation time
    // for incoming connections.
    void start_accept();

    // Called in case of an incoming connection.
    void on_new_session(const boost::system::error_code& ec, const Session::Ptr& session);

    // Interprets the given error code, carrying out maintenance on the Session::Registry
    // before rethrowing as a std::system_error.
    void handle_error_from_socket_operation_for_uid(const boost::system::error_code& ec, Uid uid);

    // The io dispatcher that this instance is associated with.
    boost::asio::io_service& io_service;
    // The endpoint that this instance is bound to.
    boost::asio::local::stream_protocol::endpoint end_point;
    // The acceptor object for handling incoming connection requests.
    boost::asio::local::stream_protocol::acceptor acceptor;

    // Helper to map pid -> process start time.
    helpers::ProcessStartTimeResolver start_time_resolver;
    // Helper for resolving a socket's peer's credentials.
    PeerCredentialsResolver peer_credentials_resolver;
    // A synchronized registry of all known sessions.
    Session::Registry::Ptr session_registry;
};

// Models the receiving end of a remote agent, meant to be used by the trust store daemon.
class CORE_TRUST_DLL_PUBLIC Skeleton
        : public core::trust::remote::Agent::Skeleton,
        public std::enable_shared_from_this<Skeleton>
{
public:
    // Just for convenience
    typedef std::shared_ptr<Skeleton> Ptr;

    // All creation-time arguments go here.
    struct Configuration
    {
        // The agent impl.
        std::shared_ptr<Agent> impl;
        // The runtime instance to associate to.
        boost::asio::io_service& io_service;
        // The endpoint in the filesystem.
        boost::asio::local::stream_protocol::endpoint endpoint;
        // Helper for resolving a pid to the process's start time.
        helpers::ProcessStartTimeResolver start_time_resolver;
        // Helper for resolving a pid to an application id.
        helpers::AppIdResolver app_id_resolver;
        // Pattern for assembling the prompt dialog's description given
        // an app id.
        std::string description_format;
    };

    static Ptr create_skeleton_for_configuration(const Configuration& configuration);

    virtual ~Skeleton();

private:
    // Constructs a new Skeleton instance, installing impl for handling actual requests.
    Skeleton(const Configuration& configuration);

    // Called to initiate an async read operation.
    void start_read();

    // Called whenever a read operation from the socket finishes.
    void on_read_finished(const boost::system::error_code& ec, std::size_t size);

    // Handles an incoming request, reaching out to the super class implementation
    // to obtain an answer to the trust request.
    core::trust::Request::Answer process_incoming_request(const posix::Request& request);

    // Request object that we read into.
    posix::Request request;
    // Helper for resolving pid -> start time.
    helpers::ProcessStartTimeResolver start_time_resolver;
    // Helper for resolving pid -> application id.
    helpers::AppIdResolver app_id_resolver;
    // Pattern for assembling the prompt dialog's description given
    // an app id.
    std::string description_pattern;
    // The endpoint in the filesystem that we are connected with.
    boost::asio::local::stream_protocol::endpoint endpoint;
    // The actual socket for communication with the service.
    boost::asio::local::stream_protocol::socket socket;
};
}
}
}
}

#endif // CORE_TRUST_REMOTE_AGENT_H_
