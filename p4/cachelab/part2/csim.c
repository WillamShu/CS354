////////////////////////////////////////////////////////////////////////////////
// Main File:        csim.c
// This File:        csim.c
// Semester:         CS 354 Fall 2016
//
// Author:           William Shu
// Email:            fshu2@wisc.edu
// CS Login:         wshu
//
//////////////////////////// 80 columns wide ///////////////////////////////////

/*
 * csim.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions.  The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss. (I examined the trace,
 *  the largest request I saw was for 8 bytes).
 *  2. Instruction loads (I) are ignored, since we are interested in evaluating
 *  trans.c in terms of its data cache performance.
 *  3. data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus an possible eviction.
 */
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"

// #define DEBUG_ON
#define ADDRESS_LENGTH 64

/* Globals set by command line args */
int verbosity = 0; /* print trace if set */
int s = 0;         /* set index bits */
int b = 0;         /* block offset bits */
int E = 0;         /* associativity */
char *trace_file = NULL;

/* Derived from command line args */
int S; /* number of sets S = 2^s In C, you can use "pow" function*/
int B; /* block size (bytes) B = 2^b In C, you can use "pow" function*/

/* Counters used to record cache statistics */
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;
/*****************************************************************************/

/* Type: Memory address
 * Use this type whenever dealing with addresses or address masks
  */
typedef unsigned long long int mem_addr_t;

/* Type: Cache_line
 * cache for each line
 */
typedef struct cache_line {
  char valid;     // whether the cache is valid
  bool space;     // whether the cache is empty
  mem_addr_t tag; // check the tag bits
  int timecount;  // the time used

} cache_line_t;

typedef cache_line_t *cache_set_t;
typedef cache_set_t *cache_t;

/* The cache we are simulating */
cache_t cache;

/*
 * initCache -
 * Allocate data structures to hold info regrading the sets and cache lines
 * use struct "cache_line_t" here
 * Initialize valid and tag field with 0s.
 * calculate S = 2^s
 * use S and E while allocating the data structures here
 *
 * @return void
 *
 */
void initCache() {
  // initialize S
  S = pow(2, s);
  // allocate memory location
  cache = malloc(sizeof(cache_set_t) * S);
  // if cache is null, then exit.
  if (cache == null) {
    exit(1);
  }
  for (int j = 0; j < S; j++) {
    cache[j] = malloc(sizeof(cache_line_t) * E);

    // set all the cache's valid status to 0
    for (int i = 0; i < E; i++) {
      cache[j][i].valid = 0;
      cache[j][i].timecount = E;
      cache[j][i].space = true;
      cache[j][i].tag = 0;
    }
  }
}

/*
 * freeCache - free each piece of memory you allocated using malloc
 * inside initCache() function
 *
 * @return void
 */
void freeCache() {
  for (int a = 0; a < S; a++) {
    free(cache[a]);
  }
  free(cache);
}

/*
 * accessData - Access data at memory address addr.
 *   If it is already in cache, increase hit_count
 *   If it is not in cache, bring it in cache, increase miss count.
 *   Also increase eviction_count if a line is evicted.
 *
 * @mem_addr_t addr the initial address to check hit
 */
void accessData(mem_addr_t addr) {
  // extract tag bit/set bit from address provided
  int setaddr = (addr << (64 - s - b)) >> (64 - s);
  int tagaddr = addr >> (s + b);
  int b = 0;
  // modify time field to implement LRU
  for (b = 0; b < E; b++) {
    cache[setaddr][b].timecount--;
  }
  // check if there is a valid cacheline
  for (int k = 0; k < E; k++) {
    if (cache[setaddr][k].valid == 1) {
      // check the tag address
      if (cache[setaddr][k].tag == tagaddr) {
        hit_count++;
        cache[setaddr][k].timecount = E;
        cache[setaddr][k].space = false;
        return;
      } else {
        miss_count++;
      }
    }
  }
  // no hit. check if an empty cache line is found
  // update miss counter

  int LRUtimecount = E;
  int line = 0;
  // traverse all cachelines
  // to check if there is empty cacheline
  for (int k = 0; k < E; k++) {
    if (cache[setaddr][k].space == 1) {
      // update all related fields
      cache[setaddr][k].valid = 1;
      cache[setaddr][k].tag = tagaddr;
      cache[setaddr][k].space = false;
      cache[setaddr][k].timecount = E;
      return;
    }
  }
  // traverse all cachelines
  for (int i = 0; i < E; i++) {
    if (cache[setaddr][i].timecount < LRUtimecount) {
      LRUtimecount = cache[setaddr][i].timecount;
      line = i;
    }
  }
  cache[setaddr][line].valid = 1;
  cache[setaddr][line].tag = tagaddr;
  cache[setaddr][line].timecount = E;
  cache[setaddr][line].space = false;
  eviction_count++;
}

/*
 * replayTrace - replays the given trace file against the cache
 * reads the input trace file line by line
 * extracts the type of each memory access : L/S/M
 * one "L" as a load i.e. 1 memory access
 * one "S" as a store i.e. 1 memory access
 * one "M" as a load followed by a store i.e. 2 memory accesses
 * Ignore instruction fetch "I"
 *
 */
void replayTrace(char *trace_fn) {
  char buf[1000];
  mem_addr_t addr = 0;
  unsigned int len = 0;
  FILE *trace_fp = fopen(trace_fn, "r");

  if (!trace_fp) {
    fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
    exit(1);
  }

  while (fgets(buf, 1000, trace_fp) != NULL) {
    if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M') {
      sscanf(buf + 3, "%llx,%u", &addr, &len);

      if (verbosity)
        printf("%c %llx,%u ", buf[1], addr, len);

      // with
      // 1. address accessed in variable - addr
      // 2. type of acccess(S/L/M)  in variable - buf[1]
      // call accessData function here depending on type of access
      if (buf[1] == 'M') {
        accessData(addr);
        accessData(addr);
      }
      if (buf[1] == 'S' || buf[1] == 'L') {
        accessData(addr);
      }
      if (verbosity)
        printf("\n");
    }
  }

  fclose(trace_fp);
}

/*
 * printUsage - Print usage info
 */
void printUsage(char *argv[]) {
  printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
  printf("Options:\n");
  printf("  -h         Print this help message.\n");
  printf("  -v         Optional verbose flag.\n");
  printf("  -s <num>   Number of set index bits.\n");
  printf("  -E <num>   Number of lines per set.\n");
  printf("  -b <num>   Number of block offset bits.\n");
  printf("  -t <file>  Trace file.\n");
  printf("\nExamples:\n");
  printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
  printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
  exit(0);
}

/*
 * main - Main routine
 */
int main(int argc, char *argv[]) {
  char c;

  // Parse the command line arguments: -h, -v, -s, -E, -b, -t
  while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
    switch (c) {
    case 's':
      s = atoi(optarg);
      break;
    case 'E':
      E = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      trace_file = optarg;
      break;
    case 'v':
      verbosity = 1;
      break;
    case 'h':
      printUsage(argv);
      exit(0);
    default:
      printUsage(argv);
      exit(1);
    }
  }

  /* Make sure that all required command line args were specified */
  if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
    printf("%s: Missing required command line argument\n", argv[0]);
    printUsage(argv);
    exit(1);
  }

  /* Initialize cache */
  initCache();

#ifdef DEBUG_ON
  printf("DEBUG: S:%u E:%u B:%u trace:%s\n", S, E, B, trace_file);
#endif

  replayTrace(trace_file);

  /* Free allocated memory */
  freeCache();

  /* Output the hit and miss statistics for the autograder */
  printSummary(hit_count, miss_count, eviction_count);
  return 0;
}
