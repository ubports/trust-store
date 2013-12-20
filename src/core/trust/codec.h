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

#ifndef CORE_TRUST_CODEC_H_
#define CORE_TRUST_CODEC_H_

#include <core/trust/request.h>

#include <org/freedesktop/dbus/codec.h>
#include <org/freedesktop/dbus/types/stl/string.h>

namespace org
{
namespace freedesktop
{
namespace dbus
{
// Defines encoding and decoding of a trust request into a dbus message.
template<>
struct Codec<core::trust::Request>
{
    inline static void encode_argument(DBusMessageIter* in, const core::trust::Request& arg)
    {
        Codec<std::string>::encode_argument(in, arg.from);
        Codec<std::uint64_t>::encode_argument(in, arg.feature);
        std::uint64_t value = std::chrono::duration_cast<std::chrono::microseconds>(arg.when.time_since_epoch()).count();
        Codec<std::uint64_t>::encode_argument(in, value);
        Codec<std::int8_t>::encode_argument(in, static_cast<std::int8_t>(arg.answer));
    }

    inline static void decode_argument(DBusMessageIter* in, core::trust::Request& arg)
    {
        Codec<std::string>::decode_argument(in, arg.from); dbus_message_iter_next(in);
        Codec<std::uint64_t>::decode_argument(in, arg.feature); dbus_message_iter_next(in);
        std::uint64_t value;
        Codec<std::uint64_t>::decode_argument(in, value); dbus_message_iter_next(in);
        arg.when = core::trust::Request::Timestamp{std::chrono::microseconds{value}};
        std::int8_t answer;
        Codec<std::int8_t>::decode_argument(in, answer);
        arg.answer = static_cast<core::trust::Request::Answer>(answer);
    }
};
}
}
}

#endif // CORE_TRUST_CODEC_H_
