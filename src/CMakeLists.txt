# Copyright © 2013 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Thomas Voss <thomas.voss@canonical.com>

find_package(Qt5Core REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Intl REQUIRED)

pkg_check_modules(DBUS_CPP dbus-cpp REQUIRED)
pkg_check_modules(DBUS dbus-1 REQUIRED)
pkg_check_modules(GLIB glib-2.0 REQUIRED)
pkg_check_modules(GOBJECT gobject-2.0 REQUIRED)
pkg_check_modules(LIBAPPARMOR libapparmor REQUIRED)
pkg_check_modules(SQLITE3 sqlite3 REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
  ${DBUS_CPP_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GOBJECT_INCLUDE_DIRS}
  ${LIBAPPARMOR_INCLUDE_DIRS}
  ${SQLITE3_INCLUDE_DIRS}
)

set(
  TRUST_STORE_AGENT_SOURCES

  # Translation helper
  core/trust/i18n.h
  core/trust/i18n.cpp

  # An agent-implementation transforming ensuring legible application ids.
  core/trust/app_id_formatting_trust_agent.h
  core/trust/app_id_formatting_trust_agent.cpp

  # An agent implementation preventing privilege escalation attacks.
  core/trust/privilege_escalation_prevention_agent.cpp

  # An agent-implementation that allows for selectively whitelisting app ids
  core/trust/white_listing_agent.cpp
  # An agent-implementation using a store instance to cache user replies.
  core/trust/cached_agent.cpp
  core/trust/cached_agent_glog_reporter.cpp

  # Agent implementations for handling request out of process.
  core/trust/remote/agent.h
  core/trust/remote/agent.cpp
  core/trust/remote/helpers.h
  core/trust/remote/helpers.cpp
  # An implementation relying on dbus
  core/trust/remote/dbus.h
  core/trust/remote/dbus.cpp
  # An implementation relying on unix sockets, to be used for
  # inclusion with the android Camera Service.
  core/trust/remote/posix.h
  core/trust/remote/posix.cpp
)

if (TRUST_STORE_MIR_AGENT_ENABLED)
  set(
    TRUST_STORE_AGENT_SOURCES ${TRUST_STORE_AGENT_SOURCES}
    # An agent-implementation leveraging Mir's trusted prompting support
    # to prompt the user for trusting an application to access a trusted
    # system service.
    core/trust/mir/agent.cpp
    core/trust/mir/click_desktop_entry_app_info_resolver.cpp
  )

  # Make sure Qt does not inject evil macros like 'signals' and 'slots'.
  add_definitions(-DQT_NO_KEYWORDS)
  # We need this for building the Qt-based prompt UI
  set(CMAKE_AUTOMOC ON)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/core/trust/mir/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/core/trust/mir/config.h @ONLY)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/core/trust/mir/prompt_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/core/trust/mir/prompt_config.h @ONLY)

  include_directories(${CMAKE_CURRENT_BINARY_DIR}/core/trust/mir/)

  add_executable(
    trust-prompt

    # Translation helper
    core/trust/i18n.h
    core/trust/i18n.cpp

    core/trust/mir/prompt_main.cpp
  )

  target_link_libraries(
    trust-prompt

    ${Intl_LIBRARIES}
    Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick
    ${Boost_LIBRARIES}
    ${PROCESS_CPP_LDFLAGS}
  )

  install(
    TARGETS trust-prompt
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  install(
    FILES core/trust/mir/prompt_main.qml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/core/trust/mir
  )
else()
  set(
    TRUST_STORE_AGENT_SOURCES ${TRUST_STORE_AGENT_SOURCES}
    core/trust/mir/not_supported.cpp
  )
endif()

add_library(
  trust-store SHARED
  
  core/trust/agent.cpp
  core/trust/expose.cpp
  core/trust/request.cpp
  core/trust/resolve.cpp
  core/trust/runtime.h
  core/trust/runtime.cpp

  # All dbus-specific headers go here
  core/trust/dbus/agent.h
  core/trust/dbus/agent.cpp
  core/trust/dbus/agent_registry.h
  core/trust/dbus/bus_factory.h
  core/trust/dbus/bus_factory.cpp
  core/trust/dbus/codec.h
  core/trust/dbus/interface.h

  # The default implementation leverages SQLite3 to persist
  # requests.
  core/trust/impl/sqlite3/store.cpp  

  # And pull in all our agent sources.
  ${TRUST_STORE_AGENT_SOURCES}
)

# Just a helper to avoid recompilation of the daemon
add_library(
  trust-stored

  core/trust/daemon.cpp

  core/trust/runtime.h
  core/trust/runtime.cpp
)

add_executable(
  trust-stored-skeleton

  core/trust/daemon_skeleton_main.cpp
)

add_executable(
  trust-stored-stub

  core/trust/daemon_stub_main.cpp
)

add_library(
  trust-store-preseed-helper

  core/trust/preseed.h
  core/trust/preseed.cpp
)

add_executable(
  trust-store-preseed

  core/trust/preseed_main.cpp
)

target_link_libraries(
  trust-store

  dbus-cpp
  xdg

  ${Boost_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${GFLAGS_LDFLAGS}
  ${GLOG_LDFLAGS}
  ${GLIB_LDFLAGS}
  ${GOBJECT_LDFLAGS}
  ${LIBAPPARMOR_LDFLAGS}
  ${MIR_CLIENT_LDFLAGS}
  ${PROCESS_CPP_LDFLAGS}
  ${SQLITE3_LIBRARIES}
)

target_link_libraries(
  trust-stored

  trust-store

  ${MIR_CLIENT_LDFLAGS}
)

target_link_libraries(
  trust-stored-skeleton

  trust-stored
)

target_link_libraries(
  trust-stored-stub

  trust-stored
)

target_link_libraries(
  trust-store-preseed-helper

  trust-store
)

target_link_libraries(
  trust-store-preseed

  trust-store-preseed-helper
)

# We compile with all symbols visible by default. For the shipping library, we strip
# out all symbols that are not in core::trust::*
set(symbol_map "${CMAKE_SOURCE_DIR}/symbols.map")
set_target_properties(
  trust-store PROPERTIES
  LINK_FLAGS "${ldflags} -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map})

set_target_properties(
  trust-store

  PROPERTIES
  VERSION ${TRUST_STORE_VERSION_MAJOR}.${TRUST_STORE_VERSION_MINOR}.${TRUST_STORE_VERSION_PATCH}
  SOVERSION ${TRUST_STORE_VERSION_MAJOR}
)

install(
  TARGETS trust-store
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  TARGETS trust-stored-skeleton
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  TARGETS trust-stored-stub
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  TARGETS trust-store-preseed
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
