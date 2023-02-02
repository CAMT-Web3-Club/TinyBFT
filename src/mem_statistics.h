#ifndef LIBBYZ_MEM_STATISTICS_H_
#define LIBBYZ_MEM_STATISTICS_H_

#include <stddef.h>
#include <stdio.h>

#ifdef PRINT_MEM_STATISTICS

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
  } while(0)

#else /* PRINT_MEM_STATISTICS */

#define MEMSTATS_CALL_STACK_PUSH(x)
#define MEMSTATS_CALL_STACK_POP()
#define MEMSTATS_TRACK_CHANGE(x)
#define MEMSTATS_RESET()

#endif /* !PRINT_MEM_STATISTICS */

#ifdef __cplusplus
extern "C" {
#endif

void call_stack_push(const char *name);

void track_memory_change(long size);

void call_stack_pop(void);

void reset_mem_stats(void);

#ifdef __cplusplus
}
#endif

#endif  // LIBBYZ_MEM_STATISTICS_H_
