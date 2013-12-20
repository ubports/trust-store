set (OLD_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
# Don't treat warnings as errors in 3rd_party/{gmock,cucumber-cpp}
string (REPLACE " -Werror " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
find_package(Gtest REQUIRED)
include_directories(
  ${GMOCK_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
)
set (CMAKE_CXX_FLAGS ${OLD_CMAKE_CXX_FLAGS})

include_directories(
  ${GTEST_INCLUDE_DIRS}
  ${PROCESS_CPP_INCLUDE_DIRS}
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
