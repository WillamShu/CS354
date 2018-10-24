/******************************************************************************
 * FILENAME: cache.h
 * AUTHOR:   crf@cs.wisc.edu <Chris Feilbach>
 * DATE:     25 Oct 2013
 * PROVIDES: Header file for cache.c.
 *****************************************************************************/
 
#ifndef CACHE_H
#define CACHE_H

#include <stdbool.h>
#include <vector>

#include "cache_structs.h"
#include "trace_file_parser.h"

// Forward declarations.
struct cache* create_cache_struct(const unsigned long size,
				  const unsigned long associativity,
				  const unsigned long block_size);

void free_cache_struct(struct cache* c);
void do_reference(const struct memory_reference m, struct cache* c);
void do_reference(const std::vector<struct memory_reference>& mr, struct cache* c);
bool was_hit(void);
void dump_cache(const struct cache* c);
#endif
