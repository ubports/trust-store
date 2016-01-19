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

#ifndef CORE_TRUST_RUNTIME_H_
#define CORE_TRUST_RUNTIME_H_

#include <core/posix/signal.h>

#include <core/dbus/bus.h>
#include <core/dbus/executor.h>

#include <boost/asio.hpp>

#include <memory>
#include <thread>
#include <vector>

namespace core
{
namespace trust
{
// A Runtime maintains a pool of workers enabling
// implementations to dispatch invocations and have their
// ready handlers automatically executed.
class Runtime
{
public:
    // Do not execute in parallel, serialize
    // accesses.
    static constexpr std::size_t concurrency_hint{2};

    // Our evil singleton pattern. Not bad though, we control the
    // entire executable and rely on automatic cleanup of static
    // instances.
    static Runtime& instance();

    Runtime();

    // Gracefully shuts down operations.
    ~Runtime() noexcept(true);


    // run blocks until either stop is called or a
    // signal requesting graceful shutdown is received.
    void run();

    // requests the runtime to shut down, does not block.
    void stop();

    // Returns a mutable reference to the underlying boost::asio::io_service
    // powering the runtime's reactor.
    boost::asio::io_service& service();

    // Creates an executor for a bus instance hooking into this Runtime instance.
    core::dbus::Executor::Ptr make_executor_for_bus(const core::dbus::Bus::Ptr& bus);

private:
    // We trap sig term to ensure a clean shutdown.
    std::shared_ptr<core::posix::SignalTrap> signal_trap;

    // Our io_service instance exposed to remote agents.
    boost::asio::io_service io_service;

    // We keep the io_service alive and introduce some artificial
    // work.
    boost::asio::io_service::work keep_alive;

    // We execute the io_service on a pool of worker threads.
    std::vector<std::thread> pool;
};
}
}

#endif // CORE_TRUST_RUNTIME_H_
