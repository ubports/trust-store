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

#include <core/trust/agent.h>
#include <core/trust/request.h>
#include <core/trust/store.h>

#include <core/dbus/codec.h>
#include <core/dbus/message_streaming_operators.h>
#include <core/dbus/types/stl/string.h>
#include <core/dbus/types/stl/tuple.h>

namespace core
{
namespace dbus
{
template<>
struct Codec<core::trust::Request::Answer>
{
    inline static void encode_argument(core::dbus::Message::Writer& writer, const core::trust::Request::Answer& arg)
    {
        writer.push_byte(static_cast<std::int8_t>(arg));
    }

    inline static void decode_argument(core::dbus::Message::Reader& reader, core::trust::Request::Answer& arg)
    {
        arg = static_cast<core::trust::Request::Answer>(reader.pop_byte());
    }
};

template<>
struct Codec<core::trust::Store::Query::Status>
{
    inline static void encode_argument(core::dbus::Message::Writer& writer, const core::trust::Store::Query::Status& arg)
    {
        writer.push_byte(static_cast<std::int8_t>(arg));
    }

    inline static void decode_argument(core::dbus::Message::Reader& reader, core::trust::Store::Query::Status& arg)
    {
        arg = static_cast<core::trust::Store::Query::Status>(reader.pop_byte());
    }
};

template<typename Tag, typename Integer>
struct Codec<core::trust::TaggedInteger<Tag, Integer>>
{
    inline static void encode_argument(core::dbus::Message::Writer& writer, const core::trust::TaggedInteger<Tag, Integer>& arg)
    {
        writer.push_uint64(arg.value);
    }

    inline static void decode_argument(core::dbus::Message::Reader& reader, core::trust::TaggedInteger<Tag, Integer>& arg)
    {
        arg.value = reader.pop_uint64();
    }
};

template<>
struct Codec<core::trust::Request>
{
    inline static void encode_argument(core::dbus::Message::Writer& writer, const core::trust::Request& arg)
    {
        writer.push_stringn(arg.from.c_str(), arg.from.size());
        writer.push_uint64(arg.feature.value);
        writer.push_uint64(std::chrono::duration_cast<core::trust::Request::Duration>(arg.when.time_since_epoch()).count());
        Codec<core::trust::Request::Answer>::encode_argument(writer, arg.answer);
    }

    inline static void decode_argument(core::dbus::Message::Reader& reader, core::trust::Request& arg)
    {
        arg.from = reader.pop_string();
        arg.feature.value = reader.pop_uint64();
        arg.when = core::trust::Request::Timestamp{core::trust::Request::Duration{reader.pop_uint64()}};
        Codec<core::trust::Request::Answer>::decode_argument(reader, arg.answer);
    }
};

template<>
struct Codec<core::trust::Agent::RequestParameters>
{
    inline static void encode_argument(core::dbus::Message::Writer& writer, const core::trust::Agent::RequestParameters& arg)
    {
        Codec<core::trust::Uid>::encode_argument(writer, arg.application.uid);
        Codec<core::trust::Pid>::encode_argument(writer, arg.application.pid);
        Codec<std::string>::encode_argument(writer, arg.application.id);
        Codec<core::trust::Feature>::encode_argument(writer, arg.feature);
        Codec<std::string>::encode_argument(writer, arg.description);
    }

    inline static void decode_argument(core::dbus::Message::Reader& reader, core::trust::Agent::RequestParameters& arg)
    {
        Codec<core::trust::Uid>::decode_argument(reader, arg.application.uid);
        Codec<core::trust::Pid>::decode_argument(reader, arg.application.pid);
        Codec<std::string>::decode_argument(reader, arg.application.id);
        Codec<core::trust::Feature>::decode_argument(reader, arg.feature);
        Codec<std::string>::decode_argument(reader, arg.description);
    }
};
}
}

#endif // CORE_TRUST_CODEC_H_
