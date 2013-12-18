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

find_package(PkgConfig)
pkg_check_modules(SQLITE3 sqlite3 REQUIRED)

include_directories(${SQLITE3_INCLUDE_DIRS})

add_library(
  trust-store SHARED
  
  core/trust/impl/sqlite3/store.cpp
)

target_link_libraries(
  trust-store

  ${Boost_LIBRARIES}
  ${SQLITE3_LIBRARIES}
)

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
