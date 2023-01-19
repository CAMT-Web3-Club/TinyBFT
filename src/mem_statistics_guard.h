#ifndef LIBBYZ_MEM_STATISTICS_GUARD_H_
#define LIBBYZ_MEM_STATISTICS_GUARD_H_

#include "mem_statistics.h"

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
