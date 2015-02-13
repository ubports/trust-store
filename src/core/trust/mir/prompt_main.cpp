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
 * Authors:
 *  Olivier Tilloy <olivier.tilloy@canonical.com>
 *  Nick Dedekind <nick.dedekind@canonical.com>
 *  Thomas Voß <thomas.voss@canonical.com>
 */

// Qt
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QLibrary>

#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>

#include <boost/program_options.hpp>
#include <boost/program_options/environment_iterator.hpp>

#include <core/trust/i18n.h>

#include <core/posix/this_process.h>

#include "prompt_config.h"
#include "prompt_main.h"

namespace cli = core::trust::mir::cli;
namespace env = core::trust::mir::env;

namespace this_env = core::posix::this_process::env;

namespace core
{
namespace trust
{
namespace mir
{
class SignalTrap : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void quit(int code)
    {
        QCoreApplication::exit(code);
    }
};
}
}
}

namespace
{
namespace testing
{
void validate_command_line_arguments(const boost::program_options::variables_map& vm)
{
    // We are throwing exceptions here, which immediately calls abort and still gives a
    // helpful error message on the terminal.
    if (vm.count(cli::option_title) == 0) throw std::logic_error
    {
        "Missing option title."
    };

    if (vm.count(cli::option_description) == 0) throw std::logic_error
    {
        "Missing option description"
    };

    if (vm.count(cli::option_server_socket) == 0) throw std::logic_error
    {
        "Missing option mir_server_socket"
    };

    std::string mir_server_socket = vm[cli::option_server_socket].as<std::string>();

    if (mir_server_socket.find("fd://") != 0) throw std::logic_error
    {
        "mir_server_socket does not being with fd://"
    };
}
}
}

int main(int argc, char** argv)
{
    boost::program_options::options_description options;
    options.add_options()
            (cli::option_server_socket, boost::program_options::value<std::string>(), "Mir server socket to connect to.")
            (cli::option_description, boost::program_options::value<std::string>(), "Extended description of the prompt.")
            (cli::option_title, boost::program_options::value<std::string>(), "Title of the prompt.")
            (cli::option_testing, "Only checks command-line parameters and does not execute any actions.")
            (cli::option_testability, "Loads the Qt Testability plugin if provided.");

    auto parsed_options = boost::program_options::command_line_parser{argc, argv}
            .options(options)
            .allow_unregistered()
            .run();

    // Consider the command line.
    boost::program_options::variables_map vm;
    boost::program_options::store(parsed_options, vm);
    boost::program_options::notify(vm);

    // And the environment for option passing.
    parsed_options = boost::program_options::parse_environment(options, "CORE_TRUST_MIR_PROMPT_");
    boost::program_options::store(parsed_options, vm);
    boost::program_options::notify(vm);

    // We just verify command line arguments in testing and immediately return.
    if (vm.count(cli::option_testing) > 0)
    {
        testing::validate_command_line_arguments(vm); return 0;
    }

    // Let's validate the arguments.
    if (vm.count(cli::option_title) == 0)
        abort(); // We have to call abort here to make sure that we get signalled.

    std::string title = vm[cli::option_title].as<std::string>();

    std::string description;
    if (vm.count(cli::option_description) > 0)
        description = vm[cli::option_description].as<std::string>();

    if (vm.count(cli::option_server_socket) > 0)
    {
        // We make sure that the env variable is not set prior to adusting it.
        this_env::unset_or_throw(env::option_mir_socket);

        this_env::set_or_throw(
                    env::option_mir_socket,
                    vm[cli::option_server_socket].as<std::string>());
    }

    // We install our default gettext domain prior to anything qt.
    core::trust::i18n::default_text_domain();

    // We install a custom message handler to silence Qt's chattiness
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext&, const QString& msg)
    {
        switch (type)
        {
        // We only handle critical and fatal messages.
        case QtCriticalMsg:
        case QtFatalMsg:
            std::cerr << qPrintable(msg) << std::endl;
            break;
        // And just drop the rest.
        default:
            break;
        }
    });

    // We already parsed the command line arguments and do not parse them
    // to the application.
    QGuiApplication app{argc, argv};

    if (vm.count(cli::option_testability) > 0 ||
        this_env::get(env::option_testability, "0") == "1")
    {
        // This initialization code is copy'n'pasted across multiple projects. We really should _not_
        // do something like that and bundle initialization routines in autopilot-qt.
        // The testability driver is only loaded by QApplication but not by QGuiApplication.
        // However, QApplication depends on QWidget which would add some unneeded overhead => Let's load the testability driver on our own.
        QLibrary testLib(QLatin1String("qttestability"));
        if (testLib.load())
        {
            typedef void (*TasInitialize)(void);
            TasInitialize initFunction = (TasInitialize)testLib.resolve("qt_testability_init");
            if (initFunction)
                initFunction();
            else
                qCritical("Library qttestability resolve failed!");
        } else
        {
            qCritical("Library qttestability load failed!");
        }
    }

    core::trust::mir::SignalTrap signal_trap;

    QQuickView* view = new QQuickView();
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->setTitle(QGuiApplication::applicationName());

    // Make some properties known to the root context.
    // The title of the dialog.
    view->rootContext()->setContextProperty(
                cli::option_title,
                title.c_str());
    // The description of the dialog.
    view->rootContext()->setContextProperty(
                    cli::option_description,
                    description.c_str());

    // Point the engine to the right directory. Please note that
    // the respective value changes with the installation state.
    view->engine()->setBaseUrl(QUrl::fromLocalFile(appDirectory()));

    view->setSource(QUrl::fromLocalFile("prompt_main.qml"));
    view->show();

    QObject::connect(
                view->rootObject(),
                SIGNAL(quit(int)),
                &signal_trap,
                SLOT(quit(int)));

    return app.exec();
}

#include "prompt_main.moc"

