cmake_minimum_required(VERSION 2.8)

project(trust-store)


set(TRUST_STORE_VERSION_MAJOR 2)
set(TRUST_STORE_VERSION_MINOR 0)
set(TRUST_STORE_VERSION_PATCH 0)

message(STATUS "${CMAKE_PROJECT_NAME} ${TRUST_STORE_VERSION_MAJOR}.${TRUST_STORE_VERSION_MINOR}.${TRUST_STORE_VERSION_PATCH}")

find_package(CoverageReport)

include(GNUInstallDirs)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -pedantic -Wextra -fPIC -fvisibility=hidden -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -fno-strict-aliasing -fvisibility=hidden -fvisibility-inlines-hidden -pedantic -Wextra -fPIC -pthread")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
IF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
ENDIF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])

find_package(PkgConfig)
find_package(Boost COMPONENTS filesystem program_options system REQUIRED)

add_subdirectory(3rd_party/xdg)

option(
  TRUST_STORE_MIR_AGENT_ENABLED
  "Enables a local agent implementation relying on Mir's trusted prompting"
  ON)

if (TRUST_STORE_MIR_AGENT_ENABLED)
  pkg_check_modules(MIR_CLIENT mirclient REQUIRED)

  include_directories(
    ${MIR_CLIENT_INCLUDE_DIRS}
  )
endif()

pkg_check_modules(GLOG libglog REQUIRED)
pkg_check_modules(PROCESS_CPP process-cpp REQUIRED)

include(CTest)

include_directories(
  include/
  3rd_party/xdg

  ${GFLAGS_INCLUDE_DIRS}
  ${GLOG_INCLUDE_DIRS}
  ${PROCESS_CPP_INCLUDE_DIRS}
)

add_subdirectory(doc)
add_subdirectory(data)
add_subdirectory(include)
add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(tests)

enable_coverage_report(trust_store_test)
