/******************************************************************************
 * FILENAME: cahcesim.c
 * AUTHOR:   crf@cs.wisc.edu <Chris Feilbach>
 * DATE:     25 Oct 2013
 * PROVIDES: A functional, trace-driven, cache simulator for a unified, 
 *           write-back, write-allocate, single cache hierarchy.
 *****************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "cache.h"
#include "trace_file_parser.h"

static void print_stats(const struct cache*, const struct cache*);

int main(int argc, char** argv)
{
	// Check argc, otherwise print usage statement.
	int retval = 0;
        std::vector<struct memory_reference> instr_vector;
        std::vector<struct memory_reference> data_vector;
	if (argc != 6)
	{
		// Print usage statement.
		printf("\n\nusage: cachesim trace_path gold_path cache_size cache_associativity cache_block_size\n\n");
		return -1;
	} else {
		// Correct number of arguments were passed in.  
		// Determine whether the inputs actually describe a valid cache.
		
		// Convert inputs from ASCII strings to ints.
		int cache_size = atoi(argv[3]);
		int cache_associativity = atoi(argv[4]);
		int cache_block_size = atoi(argv[5]);
		
		// Check to make sure all inputs are greater than 0.
		if (cache_size <= 0)
		{
			retval = -1;
			printf("ERROR: Cache size (%d) must be greater than 0.\n", cache_size);
		}
		if (cache_associativity <= 0)
		{
			retval = -1;
			printf("ERROR: Cache associativity (%d) must be greater than 0.\n",
				   cache_associativity);
		}
		if (cache_block_size <= 0)
		{
			retval = -1;
			printf("ERROR: Cache block size (%d) must be greater than 0.\n",
				   cache_block_size);
		}
		
		// Do some simple error validation: size must be bigger than block_size.
		if (cache_size < cache_block_size || cache_size < cache_associativity)
		{
			retval = -1;
			printf("ERROR: Cache size is less than cache block size (%d) or cache associativity (%d).\n",
				   cache_block_size, cache_associativity);
		}
		
		// If retval is -1 bad input was detected, just return now.
		if (retval == -1) return retval;
		
		// Validate that these cache parameters make sense for a 32-bit
	        // machine.
		struct cache* c_instr;
		c_instr = create_cache_struct((long)cache_size,(long)cache_associativity,(long)cache_block_size);
                struct cache* c_data;
                c_data = create_cache_struct((long)cache_size,(long)cache_associativity,(long)cache_block_size);
		// Open the trace file.
		open_trace_file(argv[1]);

		// Open the gold file.
	        int ignore_gold_file = strcmp("no", argv[1]);
                
		if (ignore_gold_file) open_gold_file(argv[1]);
		bool is_gold_reference = false;
		struct gold_reference gr;

		unsigned int gold_count = 0;
		unsigned int gold_mismatch = 0;

                unsigned long total_irefs = 0;

		while(is_next_reference())
		{
			// Determine if a new gold reference needs
			// to be loaded.
			if(!is_gold_reference && is_next_gold_reference() && !ignore_gold_file)
			{
				// Currently there is no gold reference,
				// and there is new one.
				if (ignore_gold_file) gr = get_next_gold_reference();
			}
			struct memory_reference mr;
			mr = get_next_reference();

                        if (mr.type == INSTRUCTION)
                        {
			        //printf("Memory reference: %x\n", mr.address);
			        //do_reference(mr,c_instr);
			        instr_vector.push_back(mr);
                                total_irefs++;

                                if (total_irefs % 1000000 == 0 && total_irefs != 0)
                                {
                                        //print_stats(c_instr,c_data);
                                        total_irefs = 0;
                                }
                        } else {
                                data_vector.push_back(mr);
                                //do_reference(mr,c_data);
                        }

			// If a gold reference exists, now is the 
			// time to see whether it matches the address,
			// and if so, whether it behaves as expected.
			if (is_gold_reference && !ignore_gold_file)
			{
				// A gold reference exists.
				// Do the addresses match?
				if (mr.address == gr.address)
				{
					// This is the gold reference.
					is_gold_reference = false;
					
					gold_count++;

					// Determine if the gold reference
					// behaved as expected.
					if (gr.type == HIT && !was_hit())
					{
						gold_mismatch++;
					}

					if (gr.type == MISS && was_hit())
					{
						gold_mismatch++;
					}
				}
			}
		}
                do_reference(instr_vector,c_instr);
                do_reference(data_vector,c_data);
                print_stats(c_instr,c_data);

		// Free the cache structure.
		free_cache_struct(c_instr);
                free_cache_struct(c_data);
	}
	return retval;
}

static void print_stats(const struct cache* c_instr,
                        const struct cache* c_data)
{
	// Print memory references and hit rates.
	double instruction_miss_rate;
	double data_read_miss_rate;
	double data_write_miss_rate;
	instruction_miss_rate = (double)(c_instr->perf.instruction_miss_count) / (double)(c_instr->perf.instruction_reference_count);
	data_read_miss_rate = (double)(c_data->perf.data_read_miss_count) / (double)(c_data->perf.data_read_reference_count);
	data_write_miss_rate = (double)(c_data->perf.data_write_miss_count) / (double)(c_data->perf.data_write_reference_count);
	printf("\nInstruction misses:\t\t%ld\n", c_instr->perf.instruction_miss_count);
	printf("Total instruction references:\t%ld\n", c_instr->perf.instruction_reference_count);

	printf("Instruction miss rate:\t\t%.2lf\%\n", instruction_miss_rate*100);
	printf("\nTotal data read misses:\t\t%ld\n", c_data->perf.data_read_miss_count);
	printf("Total data read references:\t%ld\n", c_data->perf.data_read_reference_count);

	printf("Data read miss rate:\t\t%.2lf\%\n", data_read_miss_rate*100);
	printf("\nTotal data write misses:\t%ld\n", c_data->perf.data_write_miss_count);
	printf("Total data write references:\t%ld\n", c_data->perf.data_write_reference_count);
	printf("Data write miss rate:\t\t%.2lf\%\n", data_write_miss_rate*100);
        printf("Combined data miss rate:\t%.2lf\%\n", 100.0*(c_data->perf.data_read_miss_count + c_data->perf.data_write_miss_count)/(c_data->perf.data_read_reference_count + c_data->perf.data_write_reference_count));

	// Print out data about gold references.
	//printf("\nTotal gold references:\t%d\n", gold_count);
	//printf("Total gold mismatches:\t%d\n", gold_mismatch);
}
