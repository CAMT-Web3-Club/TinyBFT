cmake_minimum_required(VERSION 3.18.4)

project(libbyzea VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-m32 -march=haswell -msse2 -mpclmul -Wno-unused-variable -Wno-unused-but-set-variable")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-m32 -march=haswell -msse2 -mpclmul -Wno-unused-variable -Wno-unused-but-set-variable")

add_library(byzea STATIC)

target_include_directories(byzea PRIVATE src)
target_include_directories(byzea PUBLIC include)
if(DEFINED MBEDTLS_INCLUDE_PATH)
    # For some reason, the EXISTS and IS_DIRECTORY operators only work as intended
    # with absolute paths.
    get_filename_component(MBEDTLS_INCLUDE_PATH ${MBEDTLS_INCLUDE_PATH} ABSOLUTE)
    if ((NOT EXISTS ${MBEDTLS_INCLUDE_PATH}) OR (NOT IS_DIRECTORY ${MBEDTLS_INCLUDE_PATH}))
        message(FATAL_ERROR "${MBEDTLS_INCLUDE_PATH}: path does not exist or is not a directory")
    endif()
    target_include_directories(byzea PRIVATE ${MBEDTLS_INCLUDE_PATH})
endif()

if (DEFINED MAX_MESSAGE_SIZE)
    target_compile_definitions(byzea PUBLIC MAX_MESSAGE_SIZE=${MAX_MESSAGE_SIZE})
endif()

if (DEFINED MAX_REPLY_SIZE)
    target_compile_definitions(byzea PUBLIC MAX_REPLY_SIZE=${MAX_REPLY_SIZE})
endif()

if (DEFINED BLOCK_SIZE)
    target_compile_definitions(byzea PUBLIC BLOCK_SIZE=${BLOCK_SIZE})
endif()

if (DEFINED MAX_NUM_REPLICAS)
    target_compile_definitions(byzea PUBLIC MAX_NUM_REPLICAS=${MAX_NUM_REPLICAS})
endif()

if (DEFINED WINDOW_SIZE)
    target_compile_definitions(byzea PUBLIC WINDOW_SIZE=${WINDOW_SIZE})
endif()

if (DEFINED CHECKPOINT_INTERVAL)
    target_compile_definitions(byzea PUBLIC CHECKPOINT_INTERVAL=${CHECKPOINT_INTERVAL})
endif()

if (DEFINED ENABLE_RECOVERY)
    target_compile_definitions(byzea PUBLIC RECOVERY=1)
endif()

if (DEFINED PRINT_MEM_STATISTICS)
    target_compile_definitions(byzea PUBLIC PRINT_MEM_STATISTICS=${PRINT_MEM_STATISTICS})
    target_sources(byzea PRIVATE
        src/mem_statistics.c
        src/malloc.c
        src/new.cc
    )
endif()

if(DEFINED MAX_NUM_CLIENTS)
	target_compile_definitions(byzea PUBLIC MAX_NUM_CLIENTS=${MAX_NUM_CLIENTS})
endif()

if(DEFINED MAX_REQUEST_SIZE)
	target_compile_definitions(byzea PUBLIC MAX_REQUEST_SIZE=${MAX_REQUEST_SIZE})
endif()

if(DEFINED TINY_BFT)
    target_compile_definitions(byzea PUBLIC
        DYNAMIC_PARTITION_TREE=${TINY_BFT}
        ALTERNATIVE_CHECKPOINT_LOG=${TINY_BFT}
        ALTERNATIVE_CHECKPOINT_RECORDS=${TINY_BFT}
        STATIC_LOG_ALLOCATOR=${TINY_BFT}
    )
    target_sources(byzea PRIVATE
        src/agreement_region.cc
        src/checkpoint_log.cc
        src/checkpoint_record.cc
        src/checkpoint_record_log.cc
        src/checkpoint_region.cc
        src/Partition.cc
        src/special_region.cc
        src/scratch_allocator.cc
    )
else()
    target_sources(byzea PRIVATE
        src/Checkpoint_rec.cc
    )
endif()

target_compile_options(byzea PRIVATE "-Wall" "-Werror=all" "-Wextra" "-Wno-unused-function")

target_sources(byzea PRIVATE
    src/Client.cc
    src/Replica.cc
    src/New_key.cc
    src/Commit.cc
    src/Message.cc
    src/Reply.cc
    src/Digest.cc
    src/dsum.cc
    src/Node.cc
    src/Request.cc
    src/Checkpoint.cc
    src/Pre_prepare.cc
    src/Req_queue.cc
    src/Prepare.cc
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
    src/libbyz.cc
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
    src/Statistics.cc
    src/Time.cc
    src/Pre_prepare_info.cc
    src/rsa_private_key.cc
    src/rsa_public_key.cc
    src/fail.c
    src/hmac.cc
    src/platform.cc
)
