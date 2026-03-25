cmake_minimum_required(VERSION 3.18.4)

project(libbyzea)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindPkgConfig)
pkg_check_modules(MBEDTLS REQUIRED mbedtls mbedcrypto mbedx509)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

set(LIBBYZEA_SOURCES
    src/Client.cc
    src/Replica.cc
    src/Message.cc
    src/Request.cc
    src/Reply.cc
    src/Digest.cc
    src/dsum.cc
    src/Node.cc
    src/Pre_prepare.cc
    src/Prepare.cc
    src/Commit.cc
    src/Checkpoint.cc
    src/Status.cc
    src/Prepared_cert.cc
    src/ITimer.cc
    src/Principal.cc
    src/Log_allocator.cc
    src/Meta_data.cc
    src/Data.cc
    src/Fetch.cc
    src/Meta_data_cert.cc
    src/State.cc
    src/View_change.cc
    src/New_view.cc
    src/View_change_ack.cc
    src/View_info.cc
    src/NV_info.cc
    src/Rep_info.cc
    src/Meta_data_d.cc
    src/Query_stable.cc
    src/Reply_stable.cc
    src/Stable_estimator.cc
    src/Time.cc
    src/Pre_prepare_info.cc
    src/rsa_private_key.cc
    src/rsa_public_key.cc
    src/hmac.cc
    src/platform.cc
    src/Checkpoint_rec.cc
    src/libbyz.cc
    src/Transport.cc
    src/Fragmentation.cc
)

add_library(byzea STATIC ${LIBBYZEA_SOURCES})

target_include_directories(byzea PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(byzea 
    ${MBEDTLS_LIBRARIES}
    pthread
)

target_compile_definitions(byzea PRIVATE STATIC_LOG_ALLOCATOR)

add_subdirectory(examples)