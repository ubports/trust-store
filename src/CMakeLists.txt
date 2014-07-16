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

pkg_check_modules(DBUS_CPP dbus-cpp REQUIRED)
pkg_check_modules(DBUS dbus-1 REQUIRED)
pkg_check_modules(SQLITE3 sqlite3 REQUIRED)

include_directories(
  ${DBUS_CPP_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${SQLITE3_INCLUDE_DIRS}
)

# Make sure Qt does not inject evil macros like 'signals' and 'slots'.
add_definitions(-DQT_NO_KEYWORDS)
# We need this for building the Qt-based prompt UI
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(
  trust-store SHARED
  
  core/trust/expose.cpp
  core/trust/request.cpp
  core/trust/resolve.cpp

  # The default implementation leverages SQLite3 to persist
  # requests.
  core/trust/impl/sqlite3/store.cpp

  # An agent-implementation leveraging Mir's trusted prompting support
  # to prompt the user for trusting an application to access a trusted
  # system service.
  core/trust/mir/agent.cpp
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/core/trust/mir/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/core/trust/mir/config.h @ONLY)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/core/trust/mir/prompt_config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/core/trust/mir/prompt_config.h @ONLY)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/core/trust/mir/)

add_executable(
  trust-prompt

  core/trust/mir/prompt_main.cpp
)

set_target_properties(
  trust-prompt
  PROPERTIES INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/core/trust/mir
)

qt5_use_modules(trust-prompt Core Gui Qml Quick)

target_link_libraries(
  trust-store

  dbus-cpp

  ${Boost_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${MIR_CLIENT_LDFLAGS}
  ${MIR_COMMON_LDFLAGS}
  ${PROCESS_CPP_LDFLAGS}
  ${SQLITE3_LIBRARIES}
)

# We compile with all symbols visible by default. For the shipping library, we strip
# out all symbols that are not in core::dbus::*
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
  TARGETS trust-prompt
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  FILES core/trust/mir/prompt_main.qml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/core/trust/mir
)
