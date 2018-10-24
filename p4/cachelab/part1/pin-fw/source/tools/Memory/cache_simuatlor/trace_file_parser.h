/******************************************************************************
 * FILENAME: trace_file_parser.h
 * AUTHOR:   crf@cs.wisc.edu <Chris Feilbach>
 * DATE:     30 Oct 2013
 * PROVIDES: Header file for trace_file_parser.h.
 *****************************************************************************/
 
#ifndef TRACE_FILE_PARSER_H
#define TRACE_FILE_PARSER_H

#include <stdio.h> // printf(), fopen(), fscanf()
#include <stdbool.h> // true and false typedefs.

// Enumeration for gold memory references.
enum GOLD_TYPE {HIT, MISS};


struct gold_reference
{
	unsigned int address;
	enum GOLD_TYPE type;
};

bool open_trace_file(const char* path);
bool open_gold_file(const char* path);
bool is_next_reference(void);
struct memory_reference get_next_reference(void);

bool is_next_gold_reference(void);
struct gold_reference get_next_gold_reference(void);

#endif

