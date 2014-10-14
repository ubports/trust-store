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

#include <core/trust/i18n.h>

#include <libintl.h>

#include <clocale>

#include <iostream>

namespace
{
static constexpr const char* this_text_domain
{
    "trust-store"
};

std::string& mutable_service_text_domain()
{
    static std::string s{this_text_domain};
    return s;
}

bool init()
{
    // Make sure that *gettext calls work correctly.
    std::setlocale(LC_ALL, "");

    ::bindtextdomain(this_text_domain, nullptr);
    ::textdomain(this_text_domain);

    return true;
}

bool is_initialized()
{
    static const bool initialized = init();
    return initialized;
}
}

std::string core::trust::i18n::default_text_domain()
{
    return this_text_domain;
}

// Returns the text domain of the service we are acting for
std::string core::trust::i18n::service_text_domain()
{
    return mutable_service_text_domain();
}

// Adjusts the text domain of the service;
void core::trust::i18n::set_service_text_domain(const std::string& domain)
{
    mutable_service_text_domain() = domain;
}

std::string core::trust::i18n::tr(const std::string& in)
{
    if (not is_initialized())
        return in;

    return ::gettext(in.c_str());
}

std::string core::trust::i18n::tr(const std::string& in, const std::string& domain)
{
    if (not is_initialized())
        return in;

    return ::dgettext(domain.c_str(), in.c_str());
}
