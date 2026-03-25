cmake_minimum_required(VERSION 3.18.4)

project(libbyzea_examples)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Link to the built library
link_directories(${CMAKE_SOURCE_DIR}/..)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/../include)
include_directories(${CMAKE_SOURCE_DIR}/../src)

# Find mbedtls using pkg-config
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(MBEDTLS REQUIRED mbedtls mbedcrypto mbedx509)
    set(MBEDTLS_INCLUDE_DIRS ${MBEDTLS_INCLUDE_DIR})
    set(MBEDTLS_LIBRARIES ${MBEDTLS_LIBRARIES})
endif()

if(NOT MBEDTLS_FOUND)
    # Fallback: find system library 
    find_library(MBEDTLS_LIB mbedtls)
    find_library(MBEDCRYPTO_LIB mbedcrypto)
    find_library(MBEDX509_LIB mbedx509)
    if(MBEDTLS_LIB AND MBEDCRYPTO_LIB AND MBEDX509_LIB)
        set(MBEDTLS_LIBRARIES 
            ${MBEDTLS_LIB}
            ${MBEDCRYPTO_LIB}
            ${MBEDX509_LIB}
        )
    else()
        message(WARNING "mbedtls not found, build may fail")
    endif()
endif()

# Simple client example
add_executable(simple_client simple_client.cc)
target_link_libraries(simple_client byzea)
target_link_libraries(simple_client ${MBEDTLS_LIBRARIES})
target_link_libraries(simple_client pthread)

# Simple replica example  
add_executable(simple_replica simple_replica.cc)
target_link_libraries(simple_replica byzea)
target_link_libraries(simple_replica ${MBEDTLS_LIBRARIES})
target_link_libraries(simple_replica pthread)

# Test client example
add_executable(test_client test_client.cc)
target_link_libraries(test_client byzea)
target_link_libraries(test_client ${MBEDTLS_LIBRARIES})
target_link_libraries(test_client pthread)
