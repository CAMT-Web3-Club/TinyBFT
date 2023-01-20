#ifndef LIBBYZ_MEM_STATISTICS_GUARD_H_
#define LIBBYZ_MEM_STATISTICS_GUARD_H_

#include "mem_statistics.h"

#ifdef PRINT_MEM_STATISTICS

#define MEM_STATS_INIT(push, pop_on_destruction) \
  MemoryStatisticsGuard mem_guard(#push, pop_on_destruction)
#define MEM_STATS_ARG mem_guard
#define MEM_STATS_ARG_INIT_PUSH(x) MemoryStatisticsGuard().push(#x),
#define MEM_STATS_GUARD_PUSH(x) mem_guard.push(#x)
#define MEM_STATS_GUARD_POP() mem_guard.pop()
#define MEM_STATS_REF MemoryStatisticsGuard& mem_guard
#define MEM_STATS_PARAM MEM_STATS_REF,
#define MEM_STATS_ARG_PUSH(x) MEM_STATS_GUARD_PUSH(x),

#else  // !PRINT_MEM_STATISTICS

#define MEM_STATS_INIT(push, pop_on_destruction)
#define MEM_STATS_ARG
#define MEM_STATS_ARG_INIT_PUSH(x)
#define MEM_STATS_GUARD_PUSH(x)
#define MEM_STATS_GUARD_POP()
#define MEM_STATS_REF
#define MEM_STATS_PARAM
#define MEM_STATS_ARG_PUSH(x)

#endif  // PRINT_MEM_STATISTICS

namespace libbyzea {
/**
 * Class allowing pushing to the mem statistics call stack trace, before
 * other members' constructors are executed.
 *
 */
class MemoryStatisticsGuard {
 public:
  MemoryStatisticsGuard() = default;
  MemoryStatisticsGuard(const MemoryStatisticsGuard& other) = delete;
  MemoryStatisticsGuard(const MemoryStatisticsGuard&& other) = delete;
  MemoryStatisticsGuard(const char* name) : pop_on_destruction_(false) {
    push(name);
  };
  MemoryStatisticsGuard(const char* name, bool pop_on_destruction)
      : pop_on_destruction_(pop_on_destruction) {
    push(name);
  }

  ~MemoryStatisticsGuard() {
    if (pop_on_destruction_) {
      pop();
    }
  }

  MemoryStatisticsGuard& operator=(const MemoryStatisticsGuard& other) = delete;
  MemoryStatisticsGuard& operator=(const MemoryStatisticsGuard&& other) =
      delete;

  MemoryStatisticsGuard& push([[maybe_unused]] const char* name) {
#ifdef PRINT_MEM_STATISTICS
    ::call_stack_push(name);
#endif
    return *this;
  }

  void pop() {
#ifdef PRINT_MEM_STATISTICS
    ::call_stack_pop();
#endif
  }

 private:
  bool pop_on_destruction_;
};
}  // namespace libbyzea
#endif  // !LIBBYZ_MEM_STATISTICS_GUARD_H_
