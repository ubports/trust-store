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

#ifndef CORE_TRUST_I18N_H_
#define CORE_TRUST_I18N_H_

#include <core/trust/visibility.h>

#include <string>

namespace core
{
namespace trust
{
namespace i18n
{
// Returns the default text domain of this package.
CORE_TRUST_DLL_PUBLIC std::string default_text_domain();
// Returns the text domain of the service we are acting for
CORE_TRUST_DLL_PUBLIC std::string service_text_domain();
// Adjusts the text domain of the service;
CORE_TRUST_DLL_PUBLIC void set_service_text_domain(const std::string& domain);
// Translates the given input string for the default domain.
CORE_TRUST_DLL_PUBLIC std::string tr(const std::string& in);
// Translates the given input string for the given domain.
CORE_TRUST_DLL_PUBLIC std::string tr(const std::string& in, const std::string& domain);
}
}
}

#endif // CORE_TRUST_I18N_H_
