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

// Implementation-specific header
#include <core/trust/mir/agent.h>
#include <core/trust/mir/config.h>

#include <core/trust/agent.h>
#include <core/trust/request.h>
#include <core/trust/store.h>

#include "test_data.h"

#include <core/posix/fork.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <thread>

namespace
{
struct MockPromptSessionVirtualTable : public core::trust::mir::PromptSessionVirtualTable
{
    MockPromptSessionVirtualTable() : core::trust::mir::PromptSessionVirtualTable{}
    {
    }

    // Retrieve a text description of the last error.
    MOCK_METHOD0(error_message, std::string());

    // Requests a new, pre-authenticated fd for associating prompt providers.
    // Returns the fd or throws std::runtime_error.
    MOCK_METHOD0(new_fd_for_prompt_provider, int());

    // Adds a prompt provider process to the prompting session, identified by its PID.
    // Returns true if addition of the prompt provider succeeded.
    MOCK_METHOD1(add_prompt_provider_sync, bool(pid_t));
};

struct MockConnectionVirtualTable : public core::trust::mir::ConnectionVirtualTable
{
    MockConnectionVirtualTable() : core::trust::mir::ConnectionVirtualTable{}
    {
    }

    // Creates a new trusted prompt session instance synchronously.
    MOCK_METHOD3(create_prompt_session_sync,
                 core::trust::mir::PromptSessionVirtualTable::Ptr(
                     // The process id of the requesting app/service
                     core::trust::Pid app_pid,
                     // Callback handling prompt session state changes.
                     MirPromptSessionStateChangeCallback,
                     // Callback context
                     void*));
};

struct MockPromptProviderHelper : public core::trust::mir::PromptProviderHelper
{
    MockPromptProviderHelper(const core::trust::mir::PromptProviderHelper::CreationArguments& args)
        : core::trust::mir::PromptProviderHelper{args}
    {
        using namespace ::testing;
        ON_CALL(*this, exec_prompt_provider_with_arguments(_))
                .WillByDefault(
                    Invoke(this, &MockPromptProviderHelper::super_exec_prompt_provider_with_arguments));
    }

    // Execs the executable provided at construction time for the arguments and
    // returns the corresponding child process.
    MOCK_METHOD1(exec_prompt_provider_with_arguments,
                 core::posix::ChildProcess(
                     const core::trust::mir::PromptProviderHelper::InvocationArguments&));

    core::posix::ChildProcess super_exec_prompt_provider_with_arguments(const core::trust::mir::PromptProviderHelper::InvocationArguments& args)
    {
        return core::trust::mir::PromptProviderHelper::exec_prompt_provider_with_arguments(args);
    }
};

struct MockTranslator
{
    MOCK_METHOD1(translate, core::trust::Request::Answer(const core::posix::wait::Result&));
};

struct MockAppInfoResolver : public core::trust::mir::AppInfoResolver
{
    MOCK_METHOD1(resolve, core::trust::mir::AppInfo(const std::string&));
};

using ParentPidResolver = core::trust::Pid(core::trust::Pid);
class MockParentPidResolver: public testing::MockFunction<ParentPidResolver> {
public:
    MockParentPidResolver() {
        using namespace testing;

        // Assume that everything is a child of init by defult.
        ON_CALL(*this, Call(_)).WillByDefault(Return(core::trust::Pid(1)));
    }

    // Unfortunately, old GMock doesn't have a convenient AsStdFunction(), thus
    // we have to implement it ourself. Use a different name so that it also builds
    // with newer GMock (I don't want to check for GMock version).
    std::function<ParentPidResolver> GetStdFunction()
    {
        return [this] (core::trust::Pid requesting_pid)
        {
            return this->Call(requesting_pid);
        };
    }
};

std::shared_ptr<MockConnectionVirtualTable> a_mocked_connection_vtable()
{
    return std::make_shared<testing::NiceMock<MockConnectionVirtualTable>>();
}

std::shared_ptr<MockPromptSessionVirtualTable> a_mocked_prompt_session_vtable()
{
    return std::make_shared<testing::NiceMock<MockPromptSessionVirtualTable>>();
}

std::shared_ptr<MockPromptProviderHelper> a_mocked_prompt_provider_calling_bin_false()
{
    return std::make_shared<MockPromptProviderHelper>(
                core::trust::mir::PromptProviderHelper::CreationArguments
                {
                    "/bin/false"
                });
}

std::shared_ptr<MockAppInfoResolver> a_mocked_app_info_resolver()
{
    return std::make_shared<MockAppInfoResolver>();
}

std::shared_ptr<MockParentPidResolver> a_mocked_parent_pid_resolver()
{
    return std::make_shared<MockParentPidResolver>();
}

}

TEST(DefaultProcessStateTranslator, throws_for_signalled_process)
{
    core::posix::wait::Result result;
    result.status = core::posix::wait::Result::Status::signaled;
    result.detail.if_signaled.signal = core::posix::Signal::sig_kill;
    result.detail.if_signaled.core_dumped = true;

    auto translator = core::trust::mir::Agent::translator_only_accepting_exit_status_success();
    EXPECT_THROW(translator(result), std::logic_error);
}

TEST(DefaultProcessStateTranslator, throws_for_stopped_process)
{
    core::posix::wait::Result result;
    result.status = core::posix::wait::Result::Status::stopped;
    result.detail.if_stopped.signal = core::posix::Signal::sig_stop;

    auto translator = core::trust::mir::Agent::translator_only_accepting_exit_status_success();
    EXPECT_THROW(translator(result), std::logic_error);
}

TEST(DefaultProcessStateTranslator, returns_denied_for_process_exiting_with_failure)
{
    core::posix::wait::Result result;
    result.status = core::posix::wait::Result::Status::exited;
    result.detail.if_exited.status = core::posix::exit::Status::failure;

    auto translator = core::trust::mir::Agent::translator_only_accepting_exit_status_success();
    EXPECT_EQ(core::trust::Request::Answer::denied, translator(result));
}

TEST(DefaultProcessStateTranslator, returns_granted_for_process_exiting_successfully)
{
    core::posix::wait::Result result;
    result.status = core::posix::wait::Result::Status::exited;
    result.detail.if_exited.status = core::posix::exit::Status::success;

    auto translator = core::trust::mir::Agent::translator_only_accepting_exit_status_success();
    EXPECT_EQ(core::trust::Request::Answer::granted, translator(result));
}

TEST(DefaultPromptProviderHelper, correctly_passes_arguments_to_prompt_executable)
{
    core::posix::this_process::env::set_or_throw("CORE_TRUST_MIR_PROMPT_TESTING", "1");

    core::trust::mir::PromptProviderHelper::CreationArguments cargs
    {
        core::trust::mir::trust_prompt_executable_in_build_dir
    };

    core::trust::mir::PromptProviderHelper::InvocationArguments iargs
    {
        42,
        {
            "/tmp",
            "Does not exist",
            "does.not.exist.application"
        },
        "Just an extended description for %1%"
    };

    core::trust::mir::PromptProviderHelper helper{cargs};
    auto child = helper.exec_prompt_provider_with_arguments(iargs);

    auto result = child.wait_for(core::posix::wait::Flags::untraced);

    EXPECT_EQ(core::posix::wait::Result::Status::exited, result.status);
    EXPECT_EQ(core::posix::exit::Status::success, result.detail.if_exited.status);

    // And clean up.
    core::posix::this_process::env::unset_or_throw("CORE_TRUST_MIR_PROMPT_TESTING");
}

TEST(MirAgent, creates_prompt_session_and_execs_helper_with_preauthenticated_fd)
{
    using namespace ::testing;

    const core::trust::Pid app_pid {21};
    const std::string app_icon{"/tmp"};
    const std::string app_name {"Does not exist"};
    const std::string app_id {"does.not.exist.application"};
    const core::trust::Feature feature{42};
    const std::string app_description {"This is just an extended description %1%"};
    const int pre_authenticated_fd {42};

    const core::trust::mir::PromptProviderHelper::InvocationArguments reference_invocation_args
    {
        pre_authenticated_fd,
        {
            app_icon,
            app_name,
            app_id
        },
        app_description
    };

    auto connection_vtable = a_mocked_connection_vtable();
    auto prompt_session_vtable = a_mocked_prompt_session_vtable();

    auto prompt_provider_exec_helper = a_mocked_prompt_provider_calling_bin_false();
    auto app_info_resolver = a_mocked_app_info_resolver();
    auto parent_pid_resolver = a_mocked_parent_pid_resolver();

    ON_CALL(*connection_vtable, create_prompt_session_sync(_, _, _))
            .WillByDefault(Return(prompt_session_vtable));

    ON_CALL(*prompt_session_vtable, error_message())
            .WillByDefault(Return(std::string()));

    ON_CALL(*prompt_session_vtable, new_fd_for_prompt_provider())
            .WillByDefault(Return(pre_authenticated_fd));

    ON_CALL(*prompt_session_vtable, add_prompt_provider_sync(_))
            .WillByDefault(Return(true));

    ON_CALL(*app_info_resolver, resolve(_))
            .WillByDefault(Return(core::trust::mir::AppInfo{app_icon, app_name, app_id}));

    EXPECT_CALL(*connection_vtable, create_prompt_session_sync(app_pid, _, _)).Times(1);
    EXPECT_CALL(*prompt_session_vtable, new_fd_for_prompt_provider()).Times(1);

    EXPECT_CALL(*prompt_provider_exec_helper,
                exec_prompt_provider_with_arguments(
                    reference_invocation_args)).Times(1);

    core::trust::mir::Agent agent
    {
        core::trust::mir::Agent::Configuration
        {
            connection_vtable,
            prompt_provider_exec_helper,
            core::trust::mir::Agent::translator_only_accepting_exit_status_success(),
            app_info_resolver,
            parent_pid_resolver->GetStdFunction(),
        }
    };

    EXPECT_EQ(core::trust::Request::Answer::denied, // /bin/false exits with failure.
              agent.authenticate_request_with_parameters(
                  core::trust::Agent::RequestParameters
                  {
                     core::trust::Uid{::getuid()},
                     app_pid,
                     app_id,
                     feature,
                     app_description
                  }));
}

TEST(MirAgent, calls_into_app_info_resolver_with_app_id)
{
    using namespace ::testing;

    const core::trust::Pid app_pid {21};
    const std::string app_id {"does.not.exist.application"};
    const std::string app_icon{"/tmp"};
    const std::string app_name{"MeMyselfAndI"};
    const core::trust::Feature feature{42};
    const std::string app_description {"This is just an extended description for %1%"};
    const int pre_authenticated_fd {42};

    const core::trust::mir::PromptProviderHelper::InvocationArguments reference_invocation_args
    {
        pre_authenticated_fd,
        {
            app_icon,
            app_name,
            app_id
        },
        app_description
    };

    auto connection_vtable = a_mocked_connection_vtable();
    auto prompt_session_vtable = a_mocked_prompt_session_vtable();

    auto prompt_provider_exec_helper = a_mocked_prompt_provider_calling_bin_false();
    auto app_info_resolver = a_mocked_app_info_resolver();
    auto parent_pid_resolver = a_mocked_parent_pid_resolver();

    ON_CALL(*connection_vtable, create_prompt_session_sync(_, _, _))
            .WillByDefault(Return(prompt_session_vtable));

    ON_CALL(*prompt_session_vtable, error_message())
            .WillByDefault(Return(std::string()));

    ON_CALL(*prompt_session_vtable, new_fd_for_prompt_provider())
            .WillByDefault(Return(pre_authenticated_fd));

    ON_CALL(*prompt_session_vtable, add_prompt_provider_sync(_))
            .WillByDefault(Return(true));

    EXPECT_CALL(*app_info_resolver, resolve(app_id)).Times(1)
            .WillRepeatedly(
                Return(core::trust::mir::AppInfo{app_icon, app_name, app_id}));
    EXPECT_CALL(*prompt_provider_exec_helper,
                exec_prompt_provider_with_arguments(
                    reference_invocation_args)).Times(1);

    core::trust::mir::Agent agent
    {
        core::trust::mir::Agent::Configuration
        {
            connection_vtable,
            prompt_provider_exec_helper,
            core::trust::mir::Agent::translator_only_accepting_exit_status_success(),
            app_info_resolver,
            parent_pid_resolver->GetStdFunction(),
        }
    };

    EXPECT_EQ(core::trust::Request::Answer::denied, // /bin/false exits with failure.
              agent.authenticate_request_with_parameters(
                  core::trust::Agent::RequestParameters
                  {
                     core::trust::Uid{::getuid()},
                     app_pid,
                     app_id,
                     feature,
                     app_description
                  }));

}

TEST(MirAgent, sig_kills_prompt_provider_process_on_status_change)
{
    using namespace ::testing;

    const core::trust::Pid app_pid {21};
    const std::string app_icon{"/tmp"};
    const std::string app_name{"MeMyselfAndI"};
    const std::string app_id {"does.not.exist.application"};
    const core::trust::Feature feature{42};
    const std::string app_description {"This is just an extended description for %1%"};
    const int pre_authenticated_fd {42};

    auto a_spinning_process = []()
    {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{20});
        }
        return core::posix::exit::Status::success;
    };

    auto connection_vtable = std::make_shared<MockConnectionVirtualTable>();
    auto prompt_session_vtable = a_mocked_prompt_session_vtable();

    auto prompt_provider_helper = std::make_shared<MockPromptProviderHelper>(
                core::trust::mir::PromptProviderHelper::CreationArguments{"/bin/false"});

    auto app_info_resolver = a_mocked_app_info_resolver();
    auto parent_pid_resolver = a_mocked_parent_pid_resolver();

    void* prompt_session_state_callback_context{nullptr};

    ON_CALL(*prompt_provider_helper, exec_prompt_provider_with_arguments(_))
            .WillByDefault(
                Return(
                    core::posix::fork(
                        a_spinning_process,
                        core::posix::StandardStream::empty)));

    ON_CALL(*prompt_session_vtable, error_message())
            .WillByDefault(Return(std::string()));

    ON_CALL(*prompt_session_vtable, new_fd_for_prompt_provider())
            .WillByDefault(
                Return(
                    pre_authenticated_fd));

    ON_CALL(*prompt_session_vtable, add_prompt_provider_sync(_))
            .WillByDefault(
                Return(
                    true));

    ON_CALL(*app_info_resolver, resolve(_))
            .WillByDefault(Return(core::trust::mir::AppInfo{app_icon, app_name, app_id}));

    // An invocation results in a session being created. In addition,
    // we store pointers to callback and context provided by the implementation
    // for being able to later trigger the callback.
    ON_CALL(*connection_vtable, create_prompt_session_sync(app_pid, _, _))
            .WillByDefault(
                DoAll(
                    SaveArg<2>(&prompt_session_state_callback_context),
                    Return(prompt_session_vtable)));

    core::trust::mir::Agent agent
    {
        core::trust::mir::Agent::Configuration
        {
            connection_vtable,
            prompt_provider_helper,
            core::trust::mir::Agent::translator_only_accepting_exit_status_success(),
            app_info_resolver,
            parent_pid_resolver->GetStdFunction(),
        }
    };

    std::thread asynchronously_stop_the_prompting_session
    {
        [&prompt_session_state_callback_context]()
        {
            std::this_thread::sleep_for(std::chrono::seconds{1});

            core::trust::mir::Agent::on_trust_session_changed_state(
                        nullptr,
                        mir_prompt_session_state_stopped,
                        prompt_session_state_callback_context);
        }
    };

    // The spinning prompt provider should get signalled if the prompting session is stopped.
    // If that does not happen, the prompt provider returns success and we would have a result
    // granted.
    EXPECT_THROW(agent.authenticate_request_with_parameters(
                     core::trust::Agent::RequestParameters
                     {
                         core::trust::Uid{::getuid()},
                         core::trust::Pid{app_pid},
                         app_id,
                         feature,
                         app_description
                     }),
                 std::logic_error);

    // And some clean up.
    if (asynchronously_stop_the_prompting_session.joinable())
        asynchronously_stop_the_prompting_session.join();
}

TEST(MirAgent, dont_exec_provider_when_unable_to_create_a_prompt_session)
{
    using namespace ::testing;

    const core::trust::Pid app_pid {21};
    const std::string app_icon{"/tmp"};
    const std::string app_name {"Does not exist"};
    const std::string app_id {"does.not.exist.application"};
    const core::trust::Feature feature{42};
    const std::string app_description {"This is just an extended description %1%"};
    const int pre_authenticated_fd {42};

    auto connection_vtable = a_mocked_connection_vtable();
    auto prompt_session_vtable = a_mocked_prompt_session_vtable();

    auto prompt_provider_exec_helper = a_mocked_prompt_provider_calling_bin_false();
    auto app_info_resolver = a_mocked_app_info_resolver();
    auto parent_pid_resolver = a_mocked_parent_pid_resolver();

    ON_CALL(*connection_vtable, create_prompt_session_sync(_, _, _))
            .WillByDefault(Return(prompt_session_vtable));

    ON_CALL(*prompt_session_vtable, error_message())
            .WillByDefault(Return(std::string("Fabricated error.")));
    ON_CALL(*prompt_session_vtable, new_fd_for_prompt_provider())
            .WillByDefault(Return(pre_authenticated_fd));

    ON_CALL(*app_info_resolver, resolve(_))
            .WillByDefault(Return(core::trust::mir::AppInfo{app_icon, app_name, app_id}));

    EXPECT_CALL(*connection_vtable, create_prompt_session_sync(app_pid, _, _)).Times(1);
    EXPECT_CALL(*prompt_session_vtable, error_message()).Times(1);
    // We have an error. New FD shouldn't be requested, and the prompt provider
    // shouldn't be exec'ed, otherwise the provider can hang.
    EXPECT_CALL(*parent_pid_resolver, Call(_)).Times(0);
    EXPECT_CALL(*prompt_session_vtable, new_fd_for_prompt_provider()).Times(0);
    EXPECT_CALL(*prompt_provider_exec_helper,
                exec_prompt_provider_with_arguments(_)).Times(0);

    core::trust::mir::Agent agent
    {
        core::trust::mir::Agent::Configuration
        {
            connection_vtable,
            prompt_provider_exec_helper,
            core::trust::mir::Agent::translator_only_accepting_exit_status_success(),
            app_info_resolver,
            parent_pid_resolver->GetStdFunction(),
        }
    };

    EXPECT_THROW(agent.authenticate_request_with_parameters(
                  core::trust::Agent::RequestParameters
                  {
                     core::trust::Uid{::getuid()},
                     app_pid,
                     app_id,
                     feature,
                     app_description
                  }), std::runtime_error);
}

TEST(MirAgent, creates_prompt_session_for_parent_pid)
{
    using namespace ::testing;

    const core::trust::Pid app_pid {21};
    const core::trust::Pid app_parent_pid {7};
    const std::string app_icon{"/tmp"};
    const std::string app_name {"Does not exist"};
    const std::string app_id {"does.not.exist.application"};
    const core::trust::Feature feature{42};
    const std::string app_description {"This is just an extended description %1%"};
    const int pre_authenticated_fd {42};

    const core::trust::mir::PromptProviderHelper::InvocationArguments reference_invocation_args
    {
        pre_authenticated_fd,
        {
            app_icon,
            app_name,
            app_id
        },
        app_description
    };

    auto connection_vtable = a_mocked_connection_vtable();
    auto prompt_session_vtable_error = a_mocked_prompt_session_vtable();
    auto prompt_session_vtable = a_mocked_prompt_session_vtable();

    auto prompt_provider_exec_helper = a_mocked_prompt_provider_calling_bin_false();
    auto app_info_resolver = a_mocked_app_info_resolver();
    auto parent_pid_resolver = a_mocked_parent_pid_resolver();

    ON_CALL(*prompt_session_vtable, error_message())
            .WillByDefault(Return(std::string()));

    ON_CALL(*prompt_session_vtable, new_fd_for_prompt_provider())
            .WillByDefault(Return(pre_authenticated_fd));

    ON_CALL(*prompt_session_vtable, add_prompt_provider_sync(_))
            .WillByDefault(Return(true));

    ON_CALL(*app_info_resolver, resolve(_))
            .WillByDefault(Return(core::trust::mir::AppInfo{app_icon, app_name, app_id}));

    EXPECT_CALL(*connection_vtable, create_prompt_session_sync(app_pid, _, _))
        .Times(1)
        .WillOnce(Return(prompt_session_vtable_error));
    EXPECT_CALL(*prompt_session_vtable_error, error_message())
        .Times(1)
        .WillOnce(Return(
            std::string("Error processing request: Could not identify application session\n")
        ));
    EXPECT_CALL(*prompt_session_vtable_error, new_fd_for_prompt_provider())
        .Times(0);

    EXPECT_CALL(*parent_pid_resolver, Call(app_pid))
        .Times(1)
        .WillOnce(Return(app_parent_pid));
    EXPECT_CALL(*connection_vtable, create_prompt_session_sync(app_parent_pid, _, _))
        .Times(1)
        .WillOnce(Return(prompt_session_vtable));

    EXPECT_CALL(*prompt_session_vtable, new_fd_for_prompt_provider()).Times(1);

    EXPECT_CALL(*prompt_provider_exec_helper,
                exec_prompt_provider_with_arguments(
                    reference_invocation_args)).Times(1);

    core::trust::mir::Agent agent
    {
        core::trust::mir::Agent::Configuration
        {
            connection_vtable,
            prompt_provider_exec_helper,
            core::trust::mir::Agent::translator_only_accepting_exit_status_success(),
            app_info_resolver,
            parent_pid_resolver->GetStdFunction(),
        }
    };

    EXPECT_EQ(core::trust::Request::Answer::denied, // /bin/false exits with failure.
              agent.authenticate_request_with_parameters(
                  core::trust::Agent::RequestParameters
                  {
                     core::trust::Uid{::getuid()},
                     app_pid,
                     app_id,
                     feature,
                     app_description
                  }));
}

TEST(TrustPrompt, aborts_for_missing_title)
{
    // And we pass in an empty argument vector
    std::vector<std::string> argv;

    // We pass in the empty env
    std::map<std::string, std::string> env;

    auto child = core::posix::exec(
                core::trust::testing::trust_prompt_executable_in_build_dir,
                argv,
                env,
                core::posix::StandardStream::empty);

    auto result = child.wait_for(core::posix::wait::Flags::untraced);

    EXPECT_EQ(core::posix::wait::Result::Status::signaled, result.status);
    EXPECT_EQ(core::posix::Signal::sig_abrt, result.detail.if_signaled.signal);
}

/***********************************************************************
* All tests requiring a running Mir instance go here.                  *
* They are tagged with _requires_mir and taken out of the              *
* automatic build and test cycle.                                      *
***********************************************************************/

#include <core/trust/mir_agent.h>

#include <core/trust/mir/config.h>
#include <core/trust/mir/prompt_main.h>

#include <xdg.h>

namespace
{
std::map<std::string, std::string> a_copy_of_the_env()
{
    std::map<std::string, std::string> result;
    core::posix::this_process::env::for_each([&result](const std::string& key, const std::string& value)
    {
        result.insert(std::make_pair(key, value)) ;
    });
    return result;
}

std::string mir_socket()
{
    // We either take the XDG_RUNTIME_DIR or fall back to /tmp if XDG_RUNTIME_DIR is not set.
    try
    {
        return xdg::runtime().dir().string() + "/mir_socket";
    } catch(...)
    {
        return "/tmp/mir_socket";
    }
}

std::string trusted_mir_socket()
{
    // We either take the XDG_RUNTIME_DIR or fall back to /tmp if XDG_RUNTIME_DIR is not set.
    try
    {
        return xdg::runtime().dir().string() + "/mir_socket_trusted";
    } catch(...)
    {
        return "/tmp/mir_socket_trusted";
    }
}
}

TEST(MirAgent, default_agent_works_correctly_against_running_mir_instance_requires_mir)
{
    std::string pretty_function{__PRETTY_FUNCTION__};

    // We start up an application in a child process and simulate that it is requesting access
    // to a trusted system service/resource.
    std::vector<std::string> argv
    {
        "--" + std::string{core::trust::mir::cli::option_server_socket} + "=" + mir_socket(),
        "--" + std::string{core::trust::mir::cli::option_icon}, pretty_function,
        "--" + std::string{core::trust::mir::cli::option_name}, pretty_function,
        "--" + std::string{core::trust::mir::cli::option_id}, pretty_function,
        "--" + std::string{core::trust::mir::cli::option_description} + "=" + pretty_function,
        // We have to circumvent unity8's authentication mechanism and just provide
        // the desktop_file_hint as part of the command line.
        "--desktop_file_hint=/usr/share/applications/webbrowser-app.desktop"
    };

    core::posix::ChildProcess app = core::posix::exec(
                core::trust::mir::trust_prompt_executable_in_lib_dir,
                argv,
                a_copy_of_the_env(),
                core::posix::StandardStream::empty);

    // We give the app some time to come up.
    std::this_thread::sleep_for(std::chrono::seconds{5});

    // We pretend to be a trusted helper and connect to mir via its trusted socket.
    auto mir_connection = mir_connect_sync(trusted_mir_socket().c_str(), pretty_function.c_str());

    // Based on the mir connection, we create a prompting agent.
    auto mir_agent = core::trust::mir::create_agent_for_mir_connection(mir_connection);

    // And issue a prompt request. As a result, the user is presented with a prompting dialog.
    auto answer = mir_agent->authenticate_request_with_parameters(
                core::trust::Agent::RequestParameters
                {
                    core::trust::Uid{::getuid()},
                    core::trust::Pid{app.pid()},
                    "embedded prompt",
                    core::trust::Feature{},
                    "embedded prompt"
                });

    // And we cross-check with the user:
    std::cout << "You answered the trust prompt with: " << answer << "."
              << "Is that correct? [y/n]:";

    char y_or_n{'n'}; std::cin >> y_or_n;
    EXPECT_EQ('y', y_or_n);

    // We are bit rude here, but we have no more use for the app.
    app.send_signal_or_throw(core::posix::Signal::sig_kill);

    // And wait for the app to complete to avoid zombies.
    app.wait_for(core::posix::wait::Flags::untraced);
}
