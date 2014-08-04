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

#include "agent.h"

#include "prompt_main.h"

// For getuid
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

namespace mir = core::trust::mir;

// Invoked whenever a request for creation of pre-authenticated fds succeeds.
void mir::PromptSessionVirtualTable::mir_client_fd_callback(MirPromptSession */*prompt_session*/, size_t count, int const* fds, void* context)
{
    if (count == 0)
        return;

    auto ctxt = static_cast<mir::PromptSessionVirtualTable::Context*>(context);

    if (not ctxt)
        return;

    ctxt->fd = fds[0];

    // Upstart enables FD_CLOEXEC by default. We have to counteract.
    if (::fcntl(ctxt->fd, F_SETFD, 0) == -1) throw std::system_error
    {
        errno,
        std::system_category()
    };
}

mir::PromptSessionVirtualTable::PromptSessionVirtualTable(MirPromptSession* prompt_session)
    : prompt_session(prompt_session)
{
}

int mir::PromptSessionVirtualTable::new_fd_for_prompt_provider()
{
    static const unsigned int fd_count = 1;

    mir::PromptSessionVirtualTable::Context context;

    mir_wait_for(mir_prompt_session_new_fds_for_prompt_providers(
                     prompt_session,
                     fd_count,
                     PromptSessionVirtualTable::mir_client_fd_callback,
                     &context));

    if (context.fd == Context::invalid_fd) throw std::runtime_error
    {
        "Could not acquire pre-authenticated file descriptors for Mir prompt session."
    };

    return context.fd;
}

void mir::PromptSessionVirtualTable::release_sync()
{
    mir_prompt_session_release_sync(prompt_session);
}

mir::ConnectionVirtualTable::ConnectionVirtualTable(MirConnection* connection)
    : connection{connection}
{
}

mir::PromptSessionVirtualTable::Ptr mir::ConnectionVirtualTable::create_prompt_session_sync(
        // The process id of the requesting app/service
        core::trust::Pid app_pid,
        // Callback handling prompt session state changes.
        mir_prompt_session_state_change_callback cb,
        // Callback context
        void* context)
{
    return PromptSessionVirtualTable::Ptr
    {
        new PromptSessionVirtualTable
        {
            mir_connection_create_prompt_session_sync(connection, app_pid.value, cb, context)
        }
    };
}

mir::PromptProviderHelper::PromptProviderHelper(
        const mir::PromptProviderHelper::CreationArguments& args) : creation_arguments(args)
{
}

core::posix::ChildProcess mir::PromptProviderHelper::exec_prompt_provider_with_arguments(
        const mir::PromptProviderHelper::InvocationArguments& args)
{
    static auto child_setup = []() {};

    std::vector<std::string> argv
    {
        "--" + std::string{core::trust::mir::cli::option_server_socket}, "fd://" + std::to_string(args.fd),
        "--" + std::string{core::trust::mir::cli::option_title}, args.application_id,
        "--" + std::string{core::trust::mir::cli::option_description}, args.description
    };

    // We just copy the environment
    std::map<std::string, std::string> env;
    core::posix::this_process::env::for_each([&env](const std::string& key, const std::string& value)
    {
        env.insert(std::make_pair(key, value));
    });


    auto result = core::posix::exec(creation_arguments.path_to_helper_executable,
                                    argv,
                                    env,
                                    core::posix::StandardStream::empty,
                                    child_setup);

    return result;
}

void mir::Agent::on_trust_session_changed_state(
        // The prompt session instance that just changed state.
        MirPromptSession* /*prompt_provider*/,
        // The new state of the prompt session instance.
        MirPromptSessionState state,
        // The context of type context.
        void* context)
{
    if (mir_prompt_session_state_stopped != state)
        return;

    auto ctxt = static_cast<mir::Agent::OnTrustSessionStateChangedCallbackContext*>(context);

    if (not ctxt)
        return;

    std::error_code ec;
    // If the trust session ended (for whatever reason), we send a SIG_KILL to the
    // prompt provider process. We hereby ensure that we never return Answer::granted
    // unless the prompt provider cleanly exited prior to the trust session stopping.
    ctxt->prompt_provider_process.send_signal(core::posix::Signal::sig_kill, ec);
    // The required wait for the child process happens in prompt_user_for_request(...).
    // TODO(tvoss): We should log ec in case of errors.
}

std::function<core::trust::Request::Answer(const core::posix::wait::Result&)> mir::Agent::translator_only_accepting_exit_status_success()
{
    return [](const core::posix::wait::Result& result) -> core::trust::Request::Answer
    {
        // We now analyze the result of the process execution.
        if (core::posix::wait::Result::Status::exited != result.status) throw std::logic_error
        {
            "The prompt provider process was signaled or stopped, "
            "unable to determine a conclusive answer from the user"
        };

        // If the child process did not exit cleanly, we deny access to the resource.
        if (core::posix::exit::Status::failure == result.detail.if_exited.status)
            return core::trust::Request::Answer::denied;

        return core::trust::Request::Answer::granted;
    };
}

mir::Agent::Agent(
        // VTable object providing access to Mir's trusted prompting functionality.
        const mir::ConnectionVirtualTable::Ptr& connection_vtable,
        // Exec helper for starting up prompt provider child processes with the correct setup
        // of command line arguments and environment variables.
        const mir::PromptProviderHelper::Ptr& exec_helper,
        // A translator function for mapping child process exit states to trust::Request answers.
        const std::function<core::trust::Request::Answer(const core::posix::wait::Result&)>& translator)
    : connection_vtable(connection_vtable),
      exec_helper(exec_helper),
      translator(translator)
{
}

// From core::trust::Agent:
core::trust::Request::Answer mir::Agent::authenticate_request_with_parameters(const core::trust::Agent::RequestParameters& parameters)
{
    // We assume that the agent implementation runs under the same user id as
    // the requesting app to prevent from cross-user attacks.
    if (core::trust::Uid{::getuid()} != parameters.application.uid) throw std::logic_error
    {
        "mir::Agent::prompt_user_for_request: current user id does not match requesting app's user id"
    };

    // We initialize our callback context with an invalid child-process for setup
    // purposes. Later on, once we have acquired a pre-authenticated fd for the
    // prompt provider, we exec the actual provider in a child process and replace the
    // instance here.
    mir::Agent::OnTrustSessionStateChangedCallbackContext cb_context
    {
        core::posix::ChildProcess::invalid()
    };

    // We ensure that the prompt session is always released cleanly, either on return or on throw.
    struct Scope
    {
        ~Scope() { prompt_session->release_sync(); }
        mir::PromptSessionVirtualTable::Ptr prompt_session;
    } scope
    {
        // We setup the prompt session and wire up to our own internal callback helper.
        connection_vtable->create_prompt_session_sync(
                    parameters.application.pid,
                    Agent::on_trust_session_changed_state,
                    &cb_context)
    };

    // Acquire a new fd for the prompt provider.
    auto fd = scope.prompt_session->new_fd_for_prompt_provider();

    // And prepare the actual execution in a child process.
    mir::PromptProviderHelper::InvocationArguments args
    {
        fd,
        parameters.application.id,
        parameters.description
    };

    // Ask the helper to fire up the prompt provider.
    cb_context.prompt_provider_process = exec_helper->exec_prompt_provider_with_arguments(args);
    // And subsequently wait for it to finish.
    auto result = cb_context.prompt_provider_process.wait_for(core::posix::wait::Flags::untraced);

    return translator(result);
}

bool mir::operator==(const mir::PromptProviderHelper::InvocationArguments& lhs, const mir::PromptProviderHelper::InvocationArguments& rhs)
{
    return std::tie(lhs.application_id, lhs.description, lhs.fd) == std::tie(rhs.application_id, rhs.description, rhs.fd);
}

#include <core/trust/mir_agent.h>

#include "config.h"

std::shared_ptr<core::trust::Agent> mir::create_agent_for_mir_connection(MirConnection* connection)
{
    mir::ConnectionVirtualTable::Ptr cvt
    {
        new mir::ConnectionVirtualTable
        {
            connection
        }
    };

    mir::PromptProviderHelper::Ptr pph
    {
        new mir::PromptProviderHelper
        {
            mir::PromptProviderHelper::CreationArguments
            {
                core::trust::mir::trust_prompt_executable_in_lib_dir
            }
        }
    };

    return mir::Agent::Ptr
    {
        new mir::Agent
        {
            cvt,
            pph,
            mir::Agent::translator_only_accepting_exit_status_success()
        }
    };
}
