cmake_minimum_required(VERSION 3.18.4)

project(libbyzea_examples)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Link to the built library
link_directories(${CMAKE_SOURCE_DIR}/../build)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/../include)
include_directories(${CMAKE_SOURCE_DIR}/../src)

# Find mbedtls - use system libraries by default
# Can be overridden with MBEDTLS_INCLUDE_PATH
if(DEFINED MBEDTLS_INCLUDE_PATH)
    get_filename_component(MBEDTLS_INCLUDE_PATH ${MBEDTLS_INCLUDE_PATH} ABSOLUTE)
    include_directories(${MBEDTLS_INCLUDE_PATH})
    set(MBEDTLS_LIBRARIES 
        ${MBEDTLS_INCLUDE_PATH}/../lib/libmbedtls.a
        ${MBEDTLS_INCLUDE_PATH}/../lib/libmbedcrypto.a
        ${MBEDTLS_INCLUDE_PATH}/../lib/libmbedx509.a
    )
else()
    # Try to find system mbedtls
    find_library(MBEDTLS_LIB mbedtls PATHS /home/phukrit7171/.local/share/containers/storage/overlay/*/diff/usr/lib)
    if(MBEDTLS_LIB)
        get_filename_component(MBEDTLS_DIR ${MBEDTLS_LIB} DIRECTORY)
        set(MBEDTLS_LIBRARIES 
            ${MBEDTLS_DIR}/libmbedtls.so.3.6.5
            ${MBEDTLS_DIR}/libmbedcrypto.so.3.6.5
            ${MBEDTLS_DIR}/libmbedx509.so.3.6.5
        )
    else()
        set(MBEDTLS_LIBRARIES mbedtls mbedcrypto mbedx509)
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
