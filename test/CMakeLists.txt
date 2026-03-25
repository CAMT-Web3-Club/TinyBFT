cmake_minimum_required(VERSION 3.18.4)

project(tinybft_test)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

set(TINYBFT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TINYBFT_ROOT_DIR}/src
    ${TINYBFT_ROOT_DIR}/include
)

set(TEST_SOURCES
    test_consensus.cc
    test_safety.cc
    test_view_change.cc
)

add_executable(tinybft_test ${TEST_SOURCES})

target_link_libraries(tinybft_test
    GTest::gtest_main
    pthread
)

gtest_discover_tests(tinybft_test)
