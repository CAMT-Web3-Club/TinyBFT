#include "mem_statistics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "th_assert.h"

#define MAX_CALL_STACK_SIZE 256

static const char *call_stack[MAX_CALL_STACK_SIZE];
static long alloc_stack[MAX_CALL_STACK_SIZE];
static long call_stack_size = 0;
static long long total = 0;
static long long max_total = 0;

#define BUF_LEN 256
static void print_mem_statistics(void) {
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

void print_total_mem(void) {
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
