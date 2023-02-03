cmake_minimum_required(VERSION 3.18.4)

project(libbyzea VERSION 0.0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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

if (DEFINED LOG_CHUNK_SIZE)
    target_compile_definitions(byzea PUBLIC LOG_CHUNK_SIZE=${LOG_CHUNK_SIZE})
endif()

if (DEFINED LOG_NUM_CHUNKS)
    target_compile_definitions(byzea PUBLIC LOG_NUM_CHUNKS=${LOG_NUM_CHUNKS})
endif()

if (DEFINED PARTITION_TREE_CHILDREN)
    target_compile_definitions(byzea PUBLIC PARTITION_TREE_CHILDREN=${PARTITION_TREE_CHILDREN})
endif()

if (DEFINED PARTITION_TREE_LEVELS)
    target_compile_definitions(byzea PUBLIC PARTITION_TREE_LEVELS=${PARTITION_TREE_LEVELS})
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

if(DEFINED LOW_MEMORY_DEMAND)
    target_compile_definitions(byzea PUBLIC
        ALTERNATIVE_CHECKPOINT_LOG=1
        ALTERNATIVE_CHECKPOINT_RECORDS=1
    )
    target_sources(byzea PRIVATE
        src/checkpoint_log.cc
        src/checkpoint_record.cc
        src/checkpoint_record_log.cc
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
    src/Big_req_table.cc
    src/Pre_prepare_info.cc
    src/rsa_private_key.cc
    src/rsa_public_key.cc
    src/fail.c
    src/umac.c
    src/platform.cc
)

enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
  unit_tests

  test/dsum_test.cc
)
target_include_directories(unit_tests PUBLIC include)
target_include_directories(unit_tests PRIVATE src)
target_link_libraries(
  unit_tests
  byzea
  mbedcrypto
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
