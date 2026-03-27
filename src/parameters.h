#ifndef _parameters_h
#define _parameters_h 1

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

// Map Kconfig values to internal macros with fallbacks
#if defined(CONFIG_TINYBFT_MAX_NUM_CLIENTS)
#define MAX_NUM_CLIENTS CONFIG_TINYBFT_MAX_NUM_CLIENTS
#elif !defined(MAX_NUM_CLIENTS)
#define MAX_NUM_CLIENTS 1
#endif

#if defined(CONFIG_TINYBFT_WINDOW_SIZE)
#define WINDOW_SIZE CONFIG_TINYBFT_WINDOW_SIZE
#elif !defined(WINDOW_SIZE)
#define WINDOW_SIZE 256
#endif

#if defined(CONFIG_TINYBFT_NUM_REPLICAS)
#define MAX_NUM_REPLICAS CONFIG_TINYBFT_NUM_REPLICAS
#elif !defined(MAX_NUM_REPLICAS)
#define MAX_NUM_REPLICAS 32
#endif

#if defined(CONFIG_TINYBFT_CHECKPOINT_INTERVAL)
#define CHECKPOINT_INTERVAL CONFIG_TINYBFT_CHECKPOINT_INTERVAL
#elif !defined(CHECKPOINT_INTERVAL)
#define CHECKPOINT_INTERVAL 128
#endif

// Important: MAX_MESSAGE_SIZE is often defined in Message.h too.
#if defined(CONFIG_TINYBFT_MAX_MESSAGE_SIZE)
#define MAX_MESSAGE_SIZE CONFIG_TINYBFT_MAX_MESSAGE_SIZE
#elif !defined(MAX_MESSAGE_SIZE)
#define MAX_MESSAGE_SIZE 16384
#endif

#ifndef MAX_REPLY_SIZE
#ifdef CONFIG_TINYBFT_MAX_REPLY_SIZE
#define MAX_REPLY_SIZE CONFIG_TINYBFT_MAX_REPLY_SIZE
#else
#define MAX_REPLY_SIZE 1024
#endif
#endif

// Narrow limits for ESP32 to fit in DRAM
#ifdef ESP_PLATFORM
#undef MAX_MESSAGE_SIZE
#define MAX_MESSAGE_SIZE 4096
#undef WINDOW_SIZE
#define WINDOW_SIZE 16
#undef CHECKPOINT_INTERVAL
#define CHECKPOINT_INTERVAL 8
#undef MAX_REPLY_SIZE
#define MAX_REPLY_SIZE 2048
#undef MAX_NUM_REPLICAS
#define MAX_NUM_REPLICAS 7
#endif

namespace libbyzea {

constexpr int Max_num_replicas = MAX_NUM_REPLICAS;
constexpr int checkpoint_interval = CHECKPOINT_INTERVAL;
constexpr int max_out = WINDOW_SIZE;
constexpr int max_num_clients = MAX_NUM_CLIENTS;
constexpr int F = Max_num_replicas / 3;
}  // namespace libbyzea

#endif  // _parameters_h
