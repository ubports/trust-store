set (OLD_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
# Don't treat warnings as errors in 3rd_party/{gmock,cucumber-cpp}
string (REPLACE " -Werror " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
find_package(Gtest REQUIRED)
include_directories(
  ${GMOCK_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
)
set (CMAKE_CXX_FLAGS ${OLD_CMAKE_CXX_FLAGS})

pkg_check_modules(DBUS dbus-1)

add_definitions(-DCORE_DBUS_ENABLE_GOOGLE_TEST_FIXTURE)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/test_data.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/test_data.h @ONLY)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GTEST_INCLUDE_DIRS}
  ${PROCESS_CPP_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
)

add_executable(
  trust_store_test
  trust_store_test.cpp
)

add_executable(
  remote_trust_store_test
  remote_trust_store_test.cpp
)

target_link_libraries(
  trust_store_test

  trust-store

  ${GTEST_BOTH_LIBRARIES}
)

target_link_libraries(
  remote_trust_store_test

  trust-store

  ${GTEST_BOTH_LIBRARIES}
  ${PROCESS_CPP_LIBRARIES}
)

add_test(trust_store_test ${CMAKE_CURRENT_BINARY_DIR}/trust_store_test)
add_test(remote_trust_store_test ${CMAKE_CURRENT_BINARY_DIR}/remote_trust_store_test)
