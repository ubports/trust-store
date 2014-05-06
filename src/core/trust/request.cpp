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

#include <core/trust/request.h>

bool core::trust::operator==(const core::trust::Request& lhs, const core::trust::Request& rhs)
{
    return lhs.from == rhs.from &&
           lhs.feature == rhs.feature &&
           lhs.when == rhs.when &&
           lhs.answer == rhs.answer;
}

std::ostream& core::trust::operator<<(std::ostream& out, const core::trust::Request::Answer& a)
{
    switch (a)
    {
    case core::trust::Request::Answer::granted: out << "granted"; break;
    case core::trust::Request::Answer::denied: out << "denied"; break;
    }

    return out;
}

std::ostream& core::trust::operator<<(std::ostream& out, const core::trust::Request& r)
{
    out << "Request("
        << "from: " << r.from << ", "
        << "feature: " << r.feature << ", "
        << "when: " << r.when.time_since_epoch().count() << ", "
        << "answer: " << r.answer << ")";

    return out;
}
