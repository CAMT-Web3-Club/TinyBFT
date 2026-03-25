cmake_minimum_required(VERSION 3.18.4)

# Auto-detect platform: ESP-IDF vs Linux
if(DEFINED ENV{IDF_PATH})
    # ESP-IDF build - delegate to ESP-IDF system
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(tinybft_app)
else()
    # Linux build
    project(libbyzea)
    
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    
    # Find mbedtls for Linux
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(MBEDTLS QUIET mbedtls mbedcrypto mbedx509)
    endif()
    
    if(NOT MBEDTLS_FOUND)
        find_library(MBEDTLS_LIB mbedtls PATHS "/nix/store/2sfycqdy94jdin4qq04yws1wgq10yz6l-mbedtls-3.6.5/lib")
        find_library(MBEDCRYPTO_LIB mbedcrypto PATHS "/nix/store/2sfycqdy94jdin4qq04yws1wgq10yz6l-mbedtls-3.6.5/lib")
        find_library(MBEDX509_LIB mbedx509 PATHS "/nix/store/2sfycqdy94jdin4qq04yws1wgq10yz6l-mbedtls-3.6.5/lib")
        if(MBEDTLS_LIB AND MBEDCRYPTO_LIB AND MBEDX509_LIB)
            set(MBEDTLS_FOUND TRUE)
            set(MBEDTLS_LIBRARIES ${MBEDTLS_LIB} ${MBEDCRYPTO_LIB} ${MBEDX509_LIB})
        endif()
    endif()
    
    if(NOT MBEDTLS_FOUND)
        message(FATAL_ERROR "mbedtls not found! Install mbedtls or use nix develop")
    endif()
    
    # Include paths
    include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    
    # All library sources
    file(GLOB LIBBYZEA_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cc")
    
    # Build static library
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
    
    # Build examples after library
    add_subdirectory(examples)
endif()