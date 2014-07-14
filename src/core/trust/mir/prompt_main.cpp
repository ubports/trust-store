/*
 * Copyright (C) 2012-2013 Canonical, Ltd.
 *
 * Authors:
 *  Olivier Tilloy <olivier.tilloy@canonical.com>
 *  Nick Dedekind <nick.dedekind@canonical.com>
 *  Thomas Voß <thomas.voss@canonical.com>
 *
 * This file is part of dialer-app.
 *
 * dialer-app is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * dialer-app is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Qt
#include <QCommandLineParser>
#include <QGuiApplication>

#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>

#include "prompt_config.h"
#include "prompt_main.h"

namespace core
{
namespace trust
{
namespace mir
{
class Prompt : public QGuiApplication
{
    Q_OBJECT
public:
    Prompt(int & argc, char ** argv)
        : QGuiApplication(argc, argv)
    {
    }

public Q_SLOTS:
    void quit(int code)
    {
        exit(code);
    }
};
}
}
}

int main(int argc, char** argv)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Trusted helper prompt");
    parser.addOption(QCommandLineOption
    {
        core::trust::mir::cli::option_server_socket,
        "Path to the server socket to connect to.",
        "socket"
    });
    parser.addOption(QCommandLineOption
    {
        core::trust::mir::cli::option_title,
        "The title of the trust prompt.",
        "title"
    });
    parser.addOption(QCommandLineOption
    {
        core::trust::mir::cli::option_description,
        "An extended description for informing the user about implications of granting trust.",
        "description"
    });

    QStringList argl;
    for (int i = 0 ; i < argc; i++) if (argv[i]) argl << argv[i];

    parser.process(argl);

    // Let's validate the arguments
    if (!parser.isSet(core::trust::mir::cli::option_title))
        abort(); // We have to call abort here to make sure that we get signalled.

    if (parser.isSet(core::trust::mir::cli::option_server_socket))
        ::setenv("MIR_SOCKET", qPrintable(parser.value(core::trust::mir::cli::option_server_socket)), 1);

    QGuiApplication::setApplicationName("Trusted Helper Prompt");
    core::trust::mir::Prompt app(argc, argv);
    QQuickView* view = new QQuickView();
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->setTitle(QGuiApplication::applicationName());
    view->rootContext()->setContextProperty("dialog", &app);
    view->rootContext()->setContextProperty(
                core::trust::mir::cli::option_title,
                parser.value(core::trust::mir::cli::option_title));
    if (parser.isSet(core::trust::mir::cli::option_description))
        view->rootContext()->setContextProperty(
                    core::trust::mir::cli::option_description,
                    parser.value(core::trust::mir::cli::option_description));
    view->engine()->setBaseUrl(QUrl::fromLocalFile(appDirectory()));

    view->setSource(QUrl::fromLocalFile("prompt_main.qml"));
    view->show();

    QObject::connect(view->rootObject(), SIGNAL(quit(int)), &app, SLOT(quit(int)));

    return app.exec();
}

#include "prompt_main.moc"

