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

#ifndef CORE_TRUST_PRESEED_H_
#define CORE_TRUST_PRESEED_H_

#include <core/trust/store.h>

#include <core/posix/exit.h>

#include <vector>

namespace core
{
namespace trust
{
// A helper executable to inject cached answers into service specific trust caches.
// Invoke it with:
//   trust-store-preseed
//   --for-service MyFunkyServiceName
//   --request "does.not.exist.app      0           granted"
//   --request "does.not.exist.app      1           granted"
//   --request "does.not.exist.app      2           denied"
//   --request "does.not.exist.app      3           granted"
//   --request "does.not.exist.app      4           granted"
//             ---------------------------------------------
//             | ---- app id ----    feature        answer |
struct Preseed
{
    // Command-line parameters, their name and their description
    struct Parameters
    {
        Parameters() = delete;

        struct ForService
        {
            static constexpr const char* name{"for-service"};
            static constexpr const char* description{"The name of the service to handle trust for."};
        };

        struct Request
        {
            static constexpr const char* name{"request"};
            static constexpr const char* description{"Requests to be seeded into the trust store. Can be specified multiple times."};
        };
    };

    // Parameters for execution of the preseed executable.
    struct Configuration
    {
        // Parses command line args and produces a configuration
        static Configuration parse_from_command_line(int argc, const char** argv);
        // The store that the answers should be inserted into.
        std::shared_ptr<Store> store;
        // The set of requests that should be preseeded into the store.
        std::vector<core::trust::Request> requests;
    };

    static core::posix::exit::Status main(const Configuration& configuration);
};
}
}

#endif // CORE_TRUST_PRESEED_H_
