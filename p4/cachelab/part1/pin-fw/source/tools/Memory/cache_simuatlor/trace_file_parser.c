/******************************************************************************
 * FILENAME: trace_file_parser.c
 * AUTHOR:   crf@cs.wisc.edu <Chris Feilbach>
 * DATE:     03 Nov 2013
 * PROVIDES: A C module that is responsible for opening a file of address traces
 *           and returning them one by one to the cache simulator.
 *
 *           This module has been extended for the TLB project in CS752.  It
 *           now also contains the ability to have a "gold" file, which contains
 *           expected hits and misses, which can be used to determine if a
 *           memory hierarchy is behaving as desired.
 *****************************************************************************/

#include "cache_structs.h"
#include "trace_file_parser.h"

static FILE* trace_file;
static FILE* gold_file;

/**********************************************************
 * bool open_trace_file(const char* path)
 * Opens a trace file.  If the file is not able to be
 * opened an error message is printed to the screen.
 *
 * ARGUMENTS:
 * x path - Path of the trace file.
 *
 * RETURN VALUE:
 * x Returns TRUE on successfully opening the trace, FALSE
 *   otherwise.
 *********************************************************/
bool open_trace_file(const char* path)
{
	trace_file = fopen(path, "r");
	
	// Check to see whether the open call failed.
	if (trace_file == NULL)
	{
		printf("ERROR: Unable to open trace file (%s)\n", path);
		return false;
	}
	
	// Trace file opened.
	return true;
}

/*********************************************************
 * bool open_gold_file(const char* path)
 * Opens a gold file.  Upon failure an error message is
 * printed to the console.
 *
 * ARGUMENTS:
 * x path - Path to gold file.
 *
 * RETURN VALUE:
 * x Returns TRUE on success, FALSE otherwise.
 *********************************************************/
bool open_gold_file(const char* path)
{
	gold_file = fopen(path, "r");
	if (gold_file == NULL)
	{
		printf("ERROR: Unable to open gold file.\n");
		return false;
	}

	return true;
}

/**********************************************************
 * bool is_next_reference(void)
 * Determines if another reference exists in the trace
 * file.
 *
 * RETURN VALUE:
 * x Returns TRUE if another memory reference exists,
 *   FALSE otherwise.
 *********************************************************/
bool is_next_reference(void)
{
	return !feof(trace_file);
}

/*********************************************************
 * bool is_next_gold_reference(void)
 * Determines if another reference exists in the gold file.
 *
 * RETURN VALUE:
 * x Returns TRUE if another memory reference exists,
 *   FALSE otherwise.
 *********************************************************/
bool is_next_gold_reference(void)
{
	return !feof(gold_file);
}

/**********************************************************
 * struct memory_reference get_next_reference(void)
 * Gets the next memory reference from the trace file of
 * the form:
 *
 * hexidecimal_address reference_type
 *
 * and converts it into a memory_reference structure.
 *
 * RETURN VALUE:
 * x A memory_reference struct containing the address of 
 *   the memory reference and type, if one exists.  If 
 *   there are no more memory references in the trace
 *   file an error messsage will be printed to stdout and
 *   the values in the struct that is returned are 
 *   undefined.
 *********************************************************/
struct memory_reference get_next_reference(void)
{
	struct memory_reference retval;
	
	// First make sure a reference exists.
	if (!is_next_reference())
	{
		printf("ERROR: Attempted to read reference from trace file when no more references exist.\n");
		return retval;
	}
	
	// Use sscanf to make life easy.
	char reference_type;
	if(fscanf(trace_file, "%lx %c\n", &retval.address, &reference_type) != 2)
	{
		// sscanf() did not read the correct number of tokens.
		printf("ERROR: sscanf() was unable to parse trace file.\n,");
		return retval;
	}
	
	// The reference_type must be I, R, or W.
	switch (reference_type)
	{
		case 'I':
			// This was an instruction fetch.
			retval.type = INSTRUCTION;
			break;
		case 'R':
			// This was a data read.
			retval.type = DATA_READ;
			break;
		case 'W':
			// This was a data write.
			retval.type = DATA_WRITE;
			break;
		default:
			// Unknown type.
			printf("ERROR: Unknown reference type detected (%c).\n", 
				   reference_type);
			break;
	}
	
	// Return the memory_reference struct.
	return retval;
}

/**********************************************************
 * struct gold_reference get_next_gold_reference(void)
 * Gets the next memory reference from the gold file.
 * Returns a structure containing information about the
 * reference.
 *
 * RETURN VALUE:
 * x Structure containing the address and type of gold 
 *   reference.
 *********************************************************/
struct gold_reference get_next_gold_reference(void)
{
	// Create the structure.
	struct gold_reference retval;

	// Determine if another reference exists.
	if (!is_next_gold_reference())
	{
		printf("ERROR: Attempted to fetch another gold reference when one did not exist.\n");
		return retval;
	}

	// Use fscanf to parse the gold file.
	char type;
	if(fscanf(gold_file, "%x %c\n", &retval.address, &type) != 2)
	{
		printf("ERROR: Scanf failed to parse gold file.\n");
		return retval;
	}

	// Gold file memory reference was read correctly.
	// Determine the type.
	switch(type)
	{
		case 'H':
			retval.type = HIT;
			break;
		case 'M':
			retval.type = MISS;
			break;
		default:
			printf("ERROR: Invalid type specified in gold file.\n");
			break;
	}

	// Return the reference.
	return retval;
}
