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

#ifndef CORE_TRUST_MIR_MIR_AGENT_H_
#define CORE_TRUST_MIR_MIR_AGENT_H_

#include <core/trust/agent.h>

#include <core/posix/child_process.h>
#include <core/posix/exec.h>

#include <mirclient/mir_toolkit/mir_client_library.h>
#include <mirclient/mir_toolkit/mir_prompt_session.h>

#include <boost/filesystem.hpp>

#include <condition_variable>
#include <functional>
#include <mutex>

namespace core
{
namespace trust
{
namespace mir
{
// Info bundles information about an application.
struct AppInfo
{
    std::string icon; // The icon of the application.
    std::string name; // The human-readable, localized name of the application.
    std::string id; // The unique id of the application.
};

// operator== returns true iff lhs is exactly equal to rhs.
bool operator==(const AppInfo& lhs, const AppInfo& rhs);

// We wrap the Mir prompt session API into a struct to
// ease with testing and mocking.
class CORE_TRUST_DLL_PUBLIC PromptSessionVirtualTable
{
public:
    // Just a convenience typedef
    typedef std::shared_ptr<PromptSessionVirtualTable> Ptr;

    // Just a helper struct to be passed to client_fd_callbacks.
    struct Context
    {
        // Marks the value of an invalid fd.
        static constexpr const int invalid_fd{-1};
        // The fd contained within this context instance.
        int fd{invalid_fd};
    };

    // Invoked whenever a request for creation of pre-authenticated fds succeeds.
    static void mir_client_fd_callback(MirPromptSession */*prompt_session*/, size_t count, int const* fds, void* context);

    // Create a MirPromptSessionVirtualTable for a given prompt session instance.
    // Please note that no change of ownwership is happening here. Instead, we expect
    // the calling code to handle object lifetimes.
    PromptSessionVirtualTable(MirPromptSession* prompt_session);
    virtual ~PromptSessionVirtualTable() = default;

    // Requests a new, pre-authenticated fd for associating prompt providers.
    // Returns the fd or throws std::runtime_error.
    virtual int new_fd_for_prompt_provider();

    // Finalizes and releases the given prompt session instance.
    virtual void release_sync();

protected:
    // Mainly used in testing to circumvent any assertions on the
    // prompt_session pointer.
    PromptSessionVirtualTable();

private:
    // The underlying prompt session instance.
    MirPromptSession* prompt_session;
};

class CORE_TRUST_DLL_PUBLIC ConnectionVirtualTable
{
public:
    // Just a convenience typedef
    typedef std::shared_ptr<ConnectionVirtualTable> Ptr;

    // Create a new instance of MirConnectionVirtualTable
    // using a pre-existing connection to Mir. Please note
    // that we do not take ownership of the MirConnection but
    // expect the calling code to coordinate object lifetimes.
    ConnectionVirtualTable(MirConnection* connection);
    virtual ~ConnectionVirtualTable() = default;

    // Creates a new trusted prompt session instance synchronously.
    virtual PromptSessionVirtualTable::Ptr create_prompt_session_sync(
            // The process id of the requesting app/service
            Pid app_pid,
            // Callback handling prompt session state changes.
            mir_prompt_session_state_change_callback cb,
            // Callback context
            void* context);
protected:
    // Mainly used in testing to circumvent any assertions on the
    // connection pointer.
    ConnectionVirtualTable();
private:
    // We do not take over ownership of the connection object.
    MirConnection* connection;
};

// Abstracts common functionality required for running external helpers.
struct CORE_TRUST_DLL_PUBLIC PromptProviderHelper
{
    // Just a convenience typedef.
    typedef std::shared_ptr<PromptProviderHelper> Ptr;

    // Creation-time arguments.
    struct CreationArguments
    {
        // Path to the helper executable that provides the prompting UI.
        std::string path_to_helper_executable;
    };

    // Invocation arguments for exec_prompt_provider_with_arguments
    struct InvocationArguments
    {
        // The pre-authenticated fd that the helper
        // should use for connecting to Mir.
        int fd;        
        // Application-specific information goes here.
        AppInfo app_info;
        // The extended description that should be presented to the user.
        std::string description;
    };

    PromptProviderHelper(const CreationArguments& args);
    virtual ~PromptProviderHelper() = default;

    // Execs the executable provided at construction time for the arguments and
    // returns the corresponding child process.
    virtual core::posix::ChildProcess exec_prompt_provider_with_arguments(const InvocationArguments& args);

    // We store all arguments passed at construction.
    CreationArguments creation_arguments;
};

// An AppNameResolver resolves an application id to a localized application name.
struct AppInfoResolver
{
    // Save us some typing.
    typedef std::shared_ptr<AppInfoResolver> Ptr;

    virtual ~AppInfoResolver() = default;
    // resolve maps app_id to a localized application name.
    virtual AppInfo resolve(const std::string& app_id) = 0;
};

// Implements the trust::Agent interface and dispatches calls to a helper
// prompt provider, tying it together with the requesting service and app
// by leveraging Mir's trusted session/prompting support.
struct CORE_TRUST_DLL_PUBLIC Agent : public core::trust::Agent
{
    // Convenience typedef
    typedef std::shared_ptr<Agent> Ptr;

    // A Configuration bundles creation-time configuration options.
    struct Configuration
    {
        // VTable object providing access to Mir's trusted prompting functionality.
        ConnectionVirtualTable::Ptr connection_vtable;
        // Exec helper for starting up prompt provider child processes with the correct setup
        // of command line arguments and environment variables.
        PromptProviderHelper::Ptr exec_helper;
        // A translator function for mapping child process exit states to trust::Request answers.
        std::function<core::trust::Request::Answer(const core::posix::wait::Result&)> translator;
        // AppNameResolver used by the agent to map incoming request app ids to application names.
        AppInfoResolver::Ptr app_info_resolver;
    };

    // Helper struct for injecting state into on_trust_changed_state_state callbacks.
    // Used in prompt_user_for_request to wait for the trust session to be stopped.
    struct OnTrustSessionStateChangedCallbackContext
    {
        // The process that provides the prompting UI.
        core::posix::ChildProcess prompt_provider_process;
    };

    // Handles state changes of trust sessions and sigkills the child process
    // provided in context (of type OnTrustSessionStateChangedCallbackContext).
    static void on_trust_session_changed_state(
            // The prompt session instance that just changed state.
            MirPromptSession* prompt_provider,
            // The new state of the prompt session instance.
            MirPromptSessionState state,
            // The context of type context.
            void* context);

    // Returns a wait result -> trust::Request::Answer translator that only returns Answer::granted if
    // the prompt provider child process exits cleanly with status success.
    // Throws std::logic_error if the process did not exit but was signaled.
    static std::function<core::trust::Request::Answer(const core::posix::wait::Result&)> translator_only_accepting_exit_status_success();

    // Creates a new MirAgent instance with the given Configuration.
    Agent(const Configuration& config);

    // From core::trust::Agent:
    // Throws a std::logic_error if anything unforeseen happens during execution, thus
    // indicating that no conclusive answer could be obtained from the user.
    core::trust::Request::Answer authenticate_request_with_parameters(const RequestParameters& parameters) override;

    // The configured options.
    Configuration config;
};

CORE_TRUST_DLL_PUBLIC bool operator==(const PromptProviderHelper::InvocationArguments&, const PromptProviderHelper::InvocationArguments&);
}
}
}

#endif // CORE_TRUST_MIR_MIR_AGENT_H_
