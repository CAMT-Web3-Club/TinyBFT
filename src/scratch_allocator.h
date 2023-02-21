#ifndef LIBBYZEA_STATIC_LOG_ALLOCATOR_H_
#define LIBBYZEA_STATIC_LOG_ALLOCATOR_H_

#include <stddef.h>

#include "parameters.h"

namespace libbyzea {

namespace scratch_allocator {

size_t memory_demand();

void *malloc(size_t size);

void *realloc(void *msg, size_t size);

void free(void *msg, size_t size);

bool is_in_scratch(void *msg);

}  // namespace scratch_allocator

}  // namespace libbyzea
#endif /* LIBBYZEA_STATIC_LOG_ALLOCATOR_H_ */
