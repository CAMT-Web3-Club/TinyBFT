cmake_minimum_required(VERSION 3.18.4)

project(libbyzea_examples)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/../include)
include_directories(${CMAKE_SOURCE_DIR}/../src)

# Find mbedtls for Linux (fallback if not via pkg-config)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(MBEDTLS QUIET mbedtls mbedcrypto mbedx509)
endif()

if(NOT MBEDTLS_FOUND)
    # Fallback: find library directly
    find_library(MBEDTLS_LIB mbedtls PATHS "/nix/store/2sfycqdy94jdin4qq04yws1wgq10yz6l-mbedtls-3.6.5/lib")
    find_library(MBEDCRYPTO_LIB mbedcrypto PATHS "/nix/store/2sfycqdy94jdin4qq04yws1wgq10yz6l-mbedtls-3.6.5/lib")
    find_library(MBEDX509_LIB mbedx509 PATHS "/nix/store/2sfycqdy94jdin4qq04yws1wgq10yz6l-mbedtls-3.6.5/lib")
    if(MBEDTLS_LIB AND MBEDCRYPTO_LIB AND MBEDX509_LIB)
        set(MBEDTLS_FOUND TRUE)
        set(MBEDTLS_LIBRARIES ${MBEDTLS_LIB} ${MBEDCRYPTO_LIB} ${MBEDX509_LIB})
    endif()
endif()

if(NOT MBEDTLS_FOUND)
    message(WARNING "mbedtls not found - linking may fail")
endif()

# Simple client example
add_executable(simple_client simple_client.cc)
target_link_libraries(simple_client byzea)
target_link_libraries(simple_client pthread)
if(MBEDTLS_FOUND)
    target_link_libraries(simple_client ${MBEDTLS_LIBRARIES})
endif()

# Simple replica example  
add_executable(simple_replica simple_replica.cc)
target_link_libraries(simple_replica byzea)
target_link_libraries(simple_replica pthread)
if(MBEDTLS_FOUND)
    target_link_libraries(simple_replica ${MBEDTLS_LIBRARIES})
endif()

# Test client example
add_executable(test_client test_client.cc)
target_link_libraries(test_client byzea)
target_link_libraries(test_client pthread)
if(MBEDTLS_FOUND)
    target_link_libraries(test_client ${MBEDTLS_LIBRARIES})
endif()