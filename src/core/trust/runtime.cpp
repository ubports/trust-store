/*
 * Copyright © 2016 Canonical Ltd.
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

#include <core/trust/runtime.h>

#include <core/dbus/asio/executor.h>

#include <iostream>
#include <stdexcept>

namespace
{
void execute_and_never_throw(boost::asio::io_service& ios) noexcept(true)
{
    while (true)
    {
        try
        {
            ios.run();
            break;
        }
        catch (const std::exception& e)
        {
            std::cerr << __PRETTY_FUNCTION__ << ": " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << __PRETTY_FUNCTION__ << ": unknown exception" << std::endl;
        }
    }
}
}

core::trust::Runtime& core::trust::Runtime::instance()
{
    static Runtime runtime;
    return runtime;
}

core::trust::Runtime::Runtime()
    : signal_trap{core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term, core::posix::Signal::sig_int})},
      keep_alive{io_service}
{
    for (std::size_t i = 0; i < Runtime::concurrency_hint; i++)
    {
        pool.emplace_back(execute_and_never_throw, std::ref(io_service));
    }

    signal_trap->signal_raised().connect([this](const core::posix::Signal&)
    {
        stop();
    });
}

core::trust::Runtime::~Runtime()
{
    io_service.stop();

    for (auto& worker : pool)
        if (worker.joinable())
            worker.join();
}

void core::trust::Runtime::run()
{
    signal_trap->run();
}

void core::trust::Runtime::stop()
{
    signal_trap->stop();
}

boost::asio::io_service& core::trust::Runtime::service()
{
    return io_service;
}

core::dbus::Executor::Ptr core::trust::Runtime::make_executor_for_bus(const core::dbus::Bus::Ptr& bus)
{
    return core::dbus::asio::make_executor(bus, io_service);
}
