#ifndef LIBBYZ_MEM_STATISTICS_H_
#define LIBBYZ_MEM_STATISTICS_H_

#include <stddef.h>
#include <stdio.h>

#ifdef PRINT_MEM_STATISTICS

#define MEMSTATS_PRINT() \
  do {                   \
    print_total_mem();   \
  } while (0)

#define MEMSTATS_SET_MEM_TYPE(x) \
  do {                           \
    set_mem_type(x);             \
  } while (0)

#define MEMSTATS_RUNTIME_LOGGING(x) \
  do {                              \
    mem_runtime_logging(x);         \
  } while (0)

#define MEMSTATS_CALL_STACK_PUSH(x) \
  do {                              \
    call_stack_push(#x);            \
  } while (0)

#define MEMSTATS_CALL_STACK_POP() \
  do {                            \
    call_stack_pop();             \
  } while (0)

#define MEMSTATS_TRACK_NEW(x)       \
  do {                              \
    call_stack_push(#x);            \
    track_memory_change(sizeof(x)); \
  } while (0)

#define MEMSTATS_TRACK_CHANGE(x) \
  do {                           \
    track_memory_change(x);      \
  } while (0)

#define MEMSTATS_RESET() \
  do {                   \
    reset_mem_stats();   \
  } while (0)

#else /* PRINT_MEM_STATISTICS */

#define MEMSTATS_PRINT()
#define MEMSTATS_SET_MEM_TYPE(x)
#define MEMSTATS_RUNTIME_LOGGING(x)
#define MEMSTATS_CALL_STACK_PUSH(x)
#define MEMSTATS_CALL_STACK_POP()
#define MEMSTATS_TRACK_CHANGE(x)
#define MEMSTATS_RESET()

#endif /* !PRINT_MEM_STATISTICS */

#ifdef __cplusplus
extern "C" {
#endif

enum mem_type {
  MEM_TYPE_NONE,
  MEM_TYPE_CERTIFICATE_LOGS,
  MEM_TYPE_LOG_ALLOCATOR,
  MEM_TYPE_STATE_MANAGEMENT,
  MEM_TYPE_VIEW_INFO,
  NUM_MEM_TYPES
};

void set_mem_type(enum mem_type type);

void mem_runtime_logging(int v);

void print_total_mem(void);

void call_stack_push(const char *name);

void track_memory_change(long size);

void call_stack_pop(void);

void reset_mem_stats(void);

#ifdef __cplusplus
}
#endif

#endif  // LIBBYZ_MEM_STATISTICS_H_
