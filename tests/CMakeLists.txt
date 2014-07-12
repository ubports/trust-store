# Build with system gmock and embedded gtest
set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")

add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/gmock")

pkg_check_modules(DBUS dbus-1)

add_definitions(-DCORE_DBUS_ENABLE_GOOGLE_TEST_FIXTURE)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/test_data.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/test_data.h @ONLY)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GMOCK_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
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

add_executable(
  request_processor_test
  request_processor_test.cpp
)

target_link_libraries(
  trust_store_test

  trust-store

  gmock

  gtest
  gtest_main
)

target_link_libraries(
  remote_trust_store_test

  trust-store

  gmock

  gtest
  gtest_main

  ${PROCESS_CPP_LIBRARIES}
)

target_link_libraries(
  request_processor_test

  trust-store

  gmock

  gtest
  gtest_main

  ${PROCESS_CPP_LIBRARIES}
)

add_test(trust_store_test ${CMAKE_CURRENT_BINARY_DIR}/trust_store_test)
add_test(remote_trust_store_test ${CMAKE_CURRENT_BINARY_DIR}/remote_trust_store_test)
add_test(request_processor_test ${CMAKE_CURRENT_BINARY_DIR}/request_processor_test)
