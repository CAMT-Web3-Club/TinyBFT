#include "mem_statistics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "th_assert.h"

#define MAX_CALL_STACK_SIZE 256

static const char *call_stack[MAX_CALL_STACK_SIZE];
static long alloc_stack[MAX_CALL_STACK_SIZE];
static long runtime_allocations[NUM_MEM_TYPES];

static long call_stack_size = 0;
static long long total = 0;
static long long max_total = 0;
static enum mem_type current_mem_type = MEM_TYPE_NONE;
static int runtime_logging = 0;

#define BUF_LEN 256
static void print_mem_statistics(void) {
  // Do not print output for functions that effectively did not allocate any
  // persistent memory.
  if (alloc_stack[call_stack_size - 1] == 0) {
    return;
  }

  static char string_buf[BUF_LEN];
  int len = 0;
  for (long i = 0; i < (call_stack_size - 1); i++) {
    len += snprintf((string_buf + len), (BUF_LEN - len), "%s->", call_stack[i]);
  }
  len += snprintf((string_buf + len), (BUF_LEN - len), "%s: %lu\n",
                  call_stack[call_stack_size - 1],
                  alloc_stack[call_stack_size - 1]);
  write(STDERR_FILENO, string_buf, len);
  fsync(STDERR_FILENO);
}

void mem_runtime_logging(int v) { runtime_logging = v; }

void set_mem_type(enum mem_type type) { current_mem_type = type; }

static char *mem_names[] = {"None",          "Certificate Logs",
                            "Log Allocator", "State Management",
                            "View Info",     "Num Mem Types"};

void print_total_mem(void) {
  long orig_size = call_stack_size;
  for (; call_stack_size >= 0; call_stack_size--) {
    print_mem_statistics();
    call_stack_size--;
  }
  call_stack_size = orig_size;
  for (int i = 0; i < NUM_MEM_TYPES; i++) {
    fprintf(stderr, "%s = %ld\n", mem_names[i], runtime_allocations[i]);
  }
  fprintf(stderr, "total = %lld\n", total);
  fprintf(stderr, "max_total = %lld\n", max_total);
}

void call_stack_push(const char *name) {
  th_assert(call_stack_size < MAX_CALL_STACK_SIZE,
            "call stack ran out of space");
  call_stack[call_stack_size] = name;
  alloc_stack[call_stack_size] = 0;
  call_stack_size++;
}

void track_memory_change(long size) {
  total += size;
  if (total > max_total) {
    max_total = total;
  }
  if (runtime_logging) {
    runtime_allocations[current_mem_type] += size;
  }

  for (long i = 0; i < call_stack_size; i++) {
    alloc_stack[i] += size;
  }
}

void call_stack_pop(void) {
  print_mem_statistics();
  call_stack_size--;
  if (call_stack_size == 0) {
    fprintf(stderr, "max_total = %lld\n", max_total);
  }
}

void reset_mem_stats(void) {
  fprintf(stderr, "### RESET STATS ###\n");
  total = 0;
  max_total = 0;
}
