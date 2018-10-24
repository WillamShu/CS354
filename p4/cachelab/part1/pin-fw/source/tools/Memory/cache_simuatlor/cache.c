/******************************************************************************
 * FILENAME: cache.c
 * AUTHOR:   crf@cs.wisc.edu <Chris Feilbach>
 * DATE:     28 Oct 2013
 * PROVIDES: A file to containing code validate cache parameters and simulate 
 *           cache memories.
 *****************************************************************************/

#include <stdio.h> // printf()
#include <stdlib.h> // malloc()
#include "cache.h"

// Static forward declarations.
static void determine_address_widths(struct cache* c);

static bool is_miss(const unsigned long tag, const unsigned int index,
                    const struct cache* c);
static void handle_miss(const unsigned long tag, const unsigned int index,
                        const bool write, const enum REFERENCE_TYPE type, const struct cache* c);
static void update_lru_info(const unsigned long tag, const unsigned int index,
                            const struct cache* c);

static unsigned long get_index_from_address(const unsigned long address,
                                           const struct cache* c);
static unsigned long get_tag_from_address(const unsigned long address,
                                         const struct cache* c);
static void determine_address_widths(struct cache* c);

static long log_two(const unsigned long n);
static bool is_power_of_two(const unsigned long n);

// hit bool.
static bool hit;
/**********************************************************
 * struct cache* create_cache_struct(const unsigned long size,
 *				     const unsigned int associativity,
 *				     const unsigned int block_size)
 *
 * This function takes in a cache size, associativity, and 
 * block size and determines whether these parameters make 
 * up a valid cache memory.
 *
 * For a cache memory to be valid the block_size must be a
 * power of two, along with the number of cache sets.  There
 * are no constraints placed on associativity and size, so long
 * as the power of two constraints are met.  This allows for
 * more advanced cache memories to be simulated if desired.
 *
 * Upon confirming that the cache parameters are valid, the 
 * function creates a cache structure and returns a pointer
 * to the structure.  If the parameters are invalid, the 
 * pointer is NULL.
 *
 * ARGUMENTS:
 * x size - Size of the cache memory.
 * x associativity - Associativity of the cache memory.
 * x block_size - Block size of the cache memory.
 *
 * RETURN VALUE:
 * x Returns a pointer to a cache structure if arguments
 *   specified represent a valid cache memory.  Returns
 *   NULL otherwise.
 *********************************************************/
struct cache* create_cache_struct(const unsigned long size,
				  const unsigned long associativity,
				  const unsigned long block_size)
{
	// If block_size is not a power of two this cache memory
	// cannot be simulated.
	if (!is_power_of_two(block_size))
	{
		printf("ERROR: Cache block size (%d) is not a power of two.\n",block_size);
		return NULL;
	}
	
	// Cache memories are organized as a collection of sets,
	// where each set can have multiple elements.  The number
	// of elements per set is the associativity of the cache.
	
	// When a cache is asked to fetch an instruction or read/write
	// data, it must first ask the question: "if this instruction
	// or data is in the cache, what set would it be in?".  The
	// index bits are used to select a set, and then the elements
	// are searched to see whether they contain the requested word.
	
	// In order use index bits in a cache memory, there must be 
	// a power of two number of sets.
	unsigned int number_of_sets = size / (associativity * block_size);
	
	// Check to see whether number_of_sets is a power of 2 and that
	// it is non-zero.
	if (number_of_sets == 0)
	{
		printf("ERROR: Number of sets (%d) must be non-zero.\n", number_of_sets);
		return NULL;
	}
	if (!is_power_of_two(number_of_sets))
	{
		printf("ERROR: Number of sets (%d) must be a power of two.\n",number_of_sets);
		return NULL;
	}
	
	// Allocate a new cache structure.
	struct cache* retval = (struct cache*)malloc(sizeof(struct cache));
	// Check malloc to make sure it actually allocated space.
	if (retval == NULL)
	{
		printf("ERROR: Failed to allocate cache structure.\n");
		return NULL;
	}
	
	// Allocate an array of cache_block arrays.  The number of 
	// arrays needed is equal to the number of sets.
	retval->blocks = (struct cache_block**)malloc(number_of_sets*sizeof(struct cache_block*));
	if (retval->blocks == NULL)
	{
		printf("ERROR: Failed to allocate array for cache_block structures.\n");
		return NULL;
	}
	
	// Now allocate the arrays for each of the sets AND set up the 
	// information to implement the LRU (Least Recently Used) replacement
	// policy.
	unsigned int i = 0;
	for(i=0;i<number_of_sets;i++)
	{
		// Allocate the array of cache_block structures.
		// The number of elements in the array is equal to
		// the associativity.
		retval->blocks[i] = (struct cache_block*)malloc(associativity*sizeof(struct cache_block));
		unsigned int j = 0;
		// Initialize each cache block.
		for(j=0;j<associativity;j++)
		{
			retval->blocks[i][j].valid = false;
			retval->blocks[i][j].dirty = false;
			retval->blocks[i][j].tag = 0;
		}
	}

	// Store the size, associativity, block size in the 
	// cache structure.
	retval->size = size;
	retval->associativity = associativity;
	retval->block_size = block_size;
	retval->number_of_sets = number_of_sets;
	
	// Determine the width of the offset, index, and tag bits.
	determine_address_widths(retval);
	
	// Set cache performance stats to 0.
	retval->perf.instruction_reference_count = 0;
	retval->perf.instruction_miss_count = 0;
	retval->perf.data_read_reference_count = 0;
	retval->perf.data_read_miss_count = 0;
	retval->perf.data_write_reference_count = 0;
	retval->perf.data_write_miss_count = 0;

	// The cache structure is set up.
	return retval;
}

/**********************************************************
 * void free_cache_struct(struct cache* c)
 * Frees any dynamically allocated memory in the cache
 * structure c.
 * 
 * ARGUMENTS
 * x c - Cache structure to free.
 *
 * RETURN VALUE
 * x None.
 *********************************************************/
void free_cache_struct(struct cache* c)
{
	unsigned int i;
	for(i=0;i<c->number_of_sets;i++)
	{
		free(c->blocks[i]);
	}
}

/**********************************************************
 * static void determine_address_widths(struct cache* c)
 * Determines the widths of the offset, index, and tag 
 * fields.  These fields are determined using the width of
 * the machine, which is inferred by getting the size of a
 * void pointer.
 *
 * c will be updated with the appropriate values in it's
 * address_info structure on success.  On failure those
 * values will contain 0s.
 * ARGUMENTS:
 * x c is the cache structure.
 *********************************************************/
static void determine_address_widths(struct cache* c)
{
	// Determine the machine width, convert to bits.
	unsigned int width = sizeof(void*) << 3;
	
	// The width of the offset is log_two(cache_block_size).
	int offset_width = log_two(c->block_size);
	// The width of the index is log_two(number_of_sets).
	int index_width = log_two(c->number_of_sets);
	// The width of the tag is machine width - offset width - index_width.
	unsigned long tag_width = width - offset_width - index_width;
	
	// Assign 0s to address_width structure in c.
	c->addr_info.address_width = 0;
	c->addr_info.offset_width = 0;
	c->addr_info.index_width = 0;
	c->addr_info.tag_width = 0;
	
	// Do error checking.
	if (offset_width == -1)
	{
		printf("ERROR: Cache block size (%d) must be a power of two.\n",c->block_size);
	} else c->addr_info.offset_width = offset_width;
	if (index_width == -1)
	{
		printf("ERROR: Number of sets in cache (%d) must be a power of two.\n", c->number_of_sets);
	} else c->addr_info.index_width = index_width;
	
	// Assign the address width and the tag.
	if (offset_width != -1 && index_width != -1)
	{
		c->addr_info.address_width = width;
		c->addr_info.tag_width = tag_width;
	}
}

/**********************************************************
 * void do_reference(const struct memory_reference m,
 *		     struct cache* c)
 *
 * This function handles a memory reference.  It determines
 * if a miss occurs, handles it if it does, updates LRU
 * information, and updates various counters indicating 
 * the number of references and misses that occurred.
 *
 * ARGUMENTS:
 * x m - Struct containing information about the memory
 *       reference to simulate.
 * x c - Structure describing the cache memory being 
 *       simulated.
 *********************************************************/
void do_reference(const struct memory_reference m, struct cache* c)
{
	// First, determine if this memory reference causes a miss.
	// Get the tag and the index from the address.
	unsigned long tag = get_tag_from_address(m.address,c);
	
	// Determine the index for this memory reference.  This determines
	// the set where the requested word *could* be.
	unsigned long index = get_index_from_address(m.address,c);

	unsigned long index_mask = (1 << c->addr_info.index_width) - 1;
	
	// Now that index and tag are known, determine if a miss will occur.
	// If a miss does happen, a different set of things need to occur
	// versus when a hit occurs.
	hit = true;
	if (is_miss(tag,index,c))
	{
		hit = false;
		// A miss occurred.  Handle it.
		// If this is a write, then set the dirty bit.
		bool is_write = false;
		if (m.type == DATA_WRITE) is_write = true;

		//printf("Miss: %x\n", m.address);		
		handle_miss(tag,index,false,m.type,c);
		
		// handle_miss sets the tag, valid, and dirty bits.
		// The LRU occurs the same way regardless of whether a cache
		// hit or cache miss occurs.
		
		// Update counters.
		// Determine type, then update appropriate counter.
		switch (m.type)
		{
		case INSTRUCTION:
			c->perf.instruction_miss_count++;
			break;
		case DATA_READ:
			c->perf.data_read_miss_count++;
			break;
		case DATA_WRITE:
			c->perf.data_write_miss_count++;
			break;
		default:
			// Do nothing.
			break;
		}
		
	} else {
		// A hit occurred.  Nothing to do.
	}
	
	// Update LRU information.
	//if (is_combined())  update_lru_info(tag,combine_index,c);
	update_lru_info(tag,index,c);
	// Update counters.
		switch (m.type)
		{
		case INSTRUCTION:
			c->perf.instruction_reference_count++;
			break;
		case DATA_READ:
			c->perf.data_read_reference_count++;
			break;
		case DATA_WRITE:
			c->perf.data_write_reference_count++;
			break;
		default:
			// Do nothing.
			break;
		}	
}

/**********************************************************
 * void do_reference(const std::vector<struct memory_reference>& mr,
 *                   struct cache* c)
 *
 * Performs a number of memory references against cache c.
 *
 * ARGUMENTS:
 * x mr - Vector containing any number of memory references.
 * x c  - Pointer to cache structure.
 * 
 *********************************************************/
void do_reference(const std::vector<struct memory_reference>& mr,
                  struct cache* c)
{
        unsigned int i;
        for(i=0;i<mr.size();i++)
        {
                do_reference(mr[i],c);
		//dump_cache(c);
        }
}

/**********************************************************
 * static bool is_miss(const unsigned long tag,
 *		       const unsigned int index,
 * 		       const struct cache* c)
 * 
 * Determines for a given cache c, whether the tag tag and
 * index index cause a miss.  This function works for 
 * direct mapped, n-way set associative, and fully 
 * associative cache memories.
 *
 * ARGUMENTS:
 * x tag   - Tag of the address of the requested word.
 * x index - Index of the address of the requested word.
 * x c     - Cache memory being simulated.
 *
 * RETURN VALUE:
 * x Returns TRUE if a cache miss occurs, false otherwise.
 *********************************************************/
static bool is_miss(const unsigned long tag, const unsigned int index,
					const struct cache* c)
{
	// Loop over the set specified by the index, and valid bits and tags.
	struct cache_block* cb;
	
	// Get the array holding the elements for the set indexed by index.
	cb = c->blocks[index];
	
	unsigned int i = 0;
	bool miss = true;
	// Loop over the elements in the set.
	for (i=0;i<c->associativity;i++)
	{
		// Get the block.
		struct cache_block temp = c->blocks[index][i];
		
		// Check the tag AND the valid bit.  If tags are 
		// equal, and valid bit is set, then a miss did
		// not occur.
		if (temp.tag == tag && temp.valid)
		{
			miss = false;
			break;
		}
	}
	
	return miss;
}

/**********************************************************
 * static void handle_miss(const unsigned long tag,
 *			   const unsigned int index,
 *			   const bool write,
 *			   const enum REFERENCE_TYPE type, 
 *			   const struct cache* c)
 *
 * This function handles when a miss occurs.  It must do
 * the following:
 * x Find the block to replace.
 * x Set the valid bit.
 * x This function does not need to update LRU information.
 *
 * ARGUMENTS:
 * x tag   - Tag of the requested word that caused the miss.
 * x index - The index of the requested word that caused the miss.
 * x c     - The struct describing the cache memory under siumulation.
 *********************************************************/
void handle_miss(const unsigned long tag, const unsigned int index,
		 const bool write, const enum REFERENCE_TYPE type,
		 const struct cache* c)
{
	// First, find the block to replace.  If the set contains
	// any elements that do not have their valid bits set, then
	// make use of those!  If not, use the LRU replacement policy to
	// decide which block to evict.
	
	// Iterate over the elements in the set indexed by index.
	// Find the LRU value that is largest.  This is the cache block
	// that has been used the least recently.
	unsigned int block_to_evict = 0;
	// Assume that the largest LRU is the first one.  This is just a guess.
	// Correct it later if it's wrong.
	unsigned int largest_LRU = c->blocks[index][0].lru_value;
	
	if (c->blocks[index][0].valid == false)
	{
		// Make use of this block.
		// Do nothing new.
	} else {
		// Loop over the remaining blocks.
		unsigned int i = 0;
		for(i=1;i<c->associativity;i++)
		{
			if (c->blocks[index][i].valid == false)
			{
				// Valid bit is not set.
				block_to_evict = i;
				break;
			} else {
				// Valid bit is set.  
				// Now check to see if the LRU value 
				// for this block is the largest found thus far.
				if (largest_LRU < c->blocks[index][i].lru_value)
				{
					// A larger LRU value was found.  
					// This means that unless an even larger
					// LRU value is found, this is the block
					// to evict.
					largest_LRU = c->blocks[index][i].lru_value;
					block_to_evict = i;
					break;
				}
			}
		}
	}
	
	// We know the block to evict now.
	// Set a new tag, valid dirty, and dirty bit if needed.
	// Do NOT UPDATE LRU INFORMATION.  This is handled by
	// the update_lru_info function.
	
	// Update the tag.
	c->blocks[index][block_to_evict].tag = tag;
	
	// Set the valid bit.
	c->blocks[index][block_to_evict].valid = true;
	
	// Update the dirty bit.
	c->blocks[index][block_to_evict].dirty = write;

	// Clear the combined bit.
	c->blocks[index][block_to_evict].combined = false;

	// Update the type.
	enum REFERENCE_TYPE temp = type;
	if (type == DATA_WRITE)
	{
		temp = DATA_READ;
	}

	c->blocks[index][block_to_evict].type = temp;
}

/**********************************************************
 * static void update_lru_info(const unsigned long tag,
 *			       const unsigned int index,
 *			       const struct cache* c)
 *
 * This function updates the LRU information for a set.
 * It takes in the tag and index of the most recent memory
 * reference, which are used to find the specific block in 
 * the cache.  It then updates all the LRU values in that set 
 * to reflect the most recent memory reference.
 *
 * ARGUMENTS:
 * x tag   - Tag word of the address of the requested word.
 * x index - Index of the address of the requested word.
 * x c     - Struct describing the cache under simulation.
 *********************************************************/
static void update_lru_info(const unsigned long tag, const unsigned int index,
			    const struct cache* c)
{
	// Determine whether the LRU data even needs to be updated.
	// LRU information is stored as follows:
	// If the value stored in the LRU field is 0, it means that
	// the block was the most recently used.  The larger the number 
	// in the LRU field, the longer the time has been since that
	// block was referenced.
	
	// Determine where the block is in the set.  This
	// function assumes that a cache hit will occur.
	unsigned int i = 0;
	unsigned int block_index = 0;
	for(i=0;i<c->associativity;i++)
	{
		// Check if this block is valid, and whether the tags match.
		if (c->blocks[index][i].valid && c->blocks[index][i].tag == tag)
		{
			// Match!
			// ERROR IN LOGIC:
			// If the LRU is 0, this means that the block is the 
			// most recently used, and no change has occurred with 
			// which block is the least recently used.
			// This leaves all LRU values at 0.

			//if (!c->blocks[index][i].lru_value) return;
			
			// Otherwise, save the block index.
			block_index = i;
		}
	}
	
	// The block index was non-zero.  Increment every element in the set,
	// and set the element in block_index to 0, to denote this is the 
	// most recently used block.
	
	// By denoting most recently used and setting it to 0, it makes it 
	// very easy to find the least recently used block ... this is the
	// one with the largest value in the LRU field in the cache_block
	// struct.
	
	for(i=0;i<c->associativity;i++)
	{
		// Increment every lru field with the exception
		// of the one stored in block_index.
		if (i == block_index) c->blocks[index][i].lru_value = 0;
		if (i != block_index) c->blocks[index][i].lru_value++;

		// For multiple hits, there may be an LRU greater than
		// the associativity. That is fine, but causes problems
		// when debugging.
		/*if (c->blocks[index][i].lru_value > c->associativity)
		{
			c->blocks[index][i].lru_value = c->associativity - 1;
		}*/
	}
	
}
 
 /*********************************************************
  * static unsigned int get_index_from_address(const unsigned int address,
  *					       const struct cache* c)
  * This function returns the index bits of the address for
  * a cache memory c.
  *
  * ARGUMENTS:
  * x address - Address to get index bits from.
  * x c       - Structure describing cache memory under
  *             simulation.
  ********************************************************/
static unsigned long get_index_from_address(const unsigned long address,
					   const struct cache* c)
{
	// Determine the number of sets, and subtract 1 to
	// turn it into a mask.
	unsigned int mask = (1 << c->addr_info.index_width) - 1;
	return (address >> c->addr_info.offset_width) & mask;
}
 
/**********************************************************
 * static unsigned int get_tag_from_address(const unsigned int address,
 *					    const struct cache* c)
 *
 * This function returns the tag bits of the address for a
 * cache memory c.
 *
 * ARGUMENTS:
 * x address - Address of the memory reference.
 * x c       - Structure describing cache memory under
 *             simulation.
 *
 * RETURN VALUE:
 * x Returns the tag of the address address.
 *********************************************************/
static unsigned long get_tag_from_address(const unsigned long address,
					 const struct cache* c)
{
	// The tag is the rightmost bits of the address.  If the
	// number of offset and index bits are calculated, then
	// a shift can be performed to get the tag.
	unsigned int bits_to_shift;
	bits_to_shift = c->addr_info.offset_width + c->addr_info.index_width;
	
	return address >> bits_to_shift;
}
 
/**********************************************************
 * static int log_two(const unsigned long n)
 * For a given number that is a power of two, log_two(n)
 * is performed.  Otherwise a value of -1 is returned,
 * indicating failure.
 *
 * ARGUMENTS:
 * x n - Input to log_two(n).
 *
 * RETURN VALUE:
 * Returns log_two(n) if n is a power of two, -1 otherwise.
 *********************************************************/
static long log_two(const unsigned long n)
{
	// Determine if this even if a power of 2 first.
	if (!is_power_of_two(n)) return -1;
	
	int power = 0;
	unsigned int i = 0;
	
	unsigned long number = n;
	
	// Find the first 1, then return it.
	for(i=0;i<sizeof(long)<<3;i++)
	{
		if ((number & 1) == 1)
		{
			power = i;
			return power;
		}
		number = number >> 1;
	}
	
	// n is not a power of two.
	return -1;
}
 
/**********************************************************
 * static bool is_power_of_two(const unsigned long n)
 * Determines if n is a power of two (ie 2^x = n, where x is
 * an integer). 
 * This can be determined by counting the number of binary 
 * bits that '1' in n.  If the answer is 1, then it is.  Any 
 * other value is false.
 * ARGUMENTS:
 * x n - Input.
 * RETURNS:
 * x TRUE if n is a power of two, FALSE otherwise.
 *********************************************************/
static bool is_power_of_two(const unsigned long n)
{
	unsigned int count = 0;
	unsigned int i = 0;
	unsigned long number = n;
	
	// Count the bits.
	for(i=0;i<sizeof(int)<<3;i++)
	{
		if (number & 1) count++;
		number = number >> 1;
	}
	
	return (count == 1);
}

/**********************************************************
 * bool was_hit(void) 
 * Determines whether the last memory reference was a 
 * cache hit or cache miss.
 *
 * RETURN VALUE:
 * x TRUE if last memory reference hit, FALSE otherwise.
 *   Return value is UNDEFINED if no memory references
 *   have been made.
 *********************************************************/
bool was_hit(void)
{
	return hit;
}

/**********************************************************
 * void dump_cache(const struct cache* c)
 * Prints out a table of the addresses in the cache to
 * stdout.
 *
 * ARGUMENTS:
 * c - Pointer to cache structure to dump.
 *
 * RETURN VALUE:
 * x None.
 *********************************************************/
void dump_cache(const struct cache* c)
{
	printf("\n\n");

	unsigned int i;
	unsigned int j;
	for(i=0;i<c->number_of_sets;i++)
	{
		unsigned int invalid = 0;
		unsigned int lru_value = 0;

		// Determine the number of invalid blocks.
		for(j=0;j<c->associativity;j++)
		{
			if (!c->blocks[i][j].valid) invalid++;
		}
		// Iterate over the set, looking for an lru_value of lru_value.
		for(lru_value=0;lru_value<c->associativity;lru_value++)
		{
			// Iterate over the entire set, looking for 
			// the lru value.
			for(j=0;j<c->associativity;j++)
			{
				if (c->blocks[i][j].lru_value == lru_value)
				{
					unsigned long tag;
					bool valid;
					tag = c->blocks[i][j].tag;
					valid = c->blocks[i][j].valid;

					if (valid) printf("%lx", tag);
					bool combined;
					combined = c->blocks[i][j].combined;
					if (valid && combined) printf("N");
					if (valid)printf("\t");
				}
			}
		}
		// Print out any invalids.
		for(j=0;j<invalid;j++)
		{
			printf("INVALID\t");
		}
		printf("\n");
	}
}
