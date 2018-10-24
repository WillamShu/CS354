/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
//
// @ORIGINAL_AUTHOR: Artur Klauser
//

/*! @file
 *  This file contains an ISA-portable PIN tool for functional simulation of
 *  instruction+data TLB+cache hieraries
 */

#include <iostream>

#include "pin.H"

#include "cache.h"
// !!! Terrible solution, but easier than hacking PIN's makefiles/reading makefile documentation
#include "cache.cpp"

// Create structs for L1 instruction and data caches.
struct cache* icache;
struct cache* dcache;

// Command line arguments.

KNOB<unsigned int> icache_size(KNOB_MODE_WRITEONCE,"pintool","is","65536","instruction cache size");
KNOB<unsigned int> icache_associativity(KNOB_MODE_WRITEONCE,"pintool","ia","1","instruction cache associativity");
KNOB<unsigned int> icache_block_size(KNOB_MODE_WRITEONCE,"pintool","ib","64","instruction cache block size");
KNOB<unsigned int> dcache_size(KNOB_MODE_WRITEONCE,"pintool","ds","65536","data cache size");
KNOB<unsigned int> dcache_associativity(KNOB_MODE_WRITEONCE,"pintool","da","1","data cache associativity");
KNOB<unsigned int> dcache_block_size(KNOB_MODE_WRITEONCE,"pintool","db","64","data cache block size");


LOCALFUN VOID Fini(int code, VOID * v)
{
    // Print out instruction cache results.
    printf("Instruction cache misses:\t%ld\n", icache->perf.instruction_miss_count);
    printf("Instruction cache references:\t%ld\n", icache->perf.instruction_reference_count);
    
    // Print out data cache results.
    printf("Data cache misses:\t\t%ld\n", dcache->perf.data_read_miss_count + dcache->perf.data_write_miss_count);
    printf("Data cache references:\t\t%ld\n", dcache->perf.data_read_reference_count + dcache->perf.data_write_reference_count);
    fflush(stdout);

    // Free cache structs.
    free_cache_struct(icache);
    free_cache_struct(dcache);
}

LOCALFUN VOID InsRef(ADDRINT addr)
{
     // Create a memory_reference structure, populate it, simulate.
     struct memory_reference mr;
     mr.address = (unsigned long)addr;
     mr.type = INSTRUCTION;
     do_reference(mr, icache);
}

LOCALFUN VOID MemRefMulti(ADDRINT addr, UINT32 size, unsigned char access_type)
{
     // Create memory_reference struct, populate, simulate reference.
     struct memory_reference mr;
     
     mr.address = (unsigned long)addr;
     mr.type = (access_type == 'l') ? DATA_READ : DATA_WRITE;
     do_reference(mr,dcache);
}

LOCALFUN VOID MemRefSingle(ADDRINT addr, UINT32 size, unsigned char access_type)
{
     // Create memory_reference struct, populate, simulate reference.
     struct memory_reference mr;
     
     mr.address = (unsigned long)addr;
     mr.type = (access_type == 'l') ? DATA_READ : DATA_WRITE;
     do_reference(mr,dcache);
}

LOCALFUN VOID Instruction(INS ins, VOID *v)
{
    // all instruction fetches access I-cache
    INS_InsertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)InsRef,
        IARG_INST_PTR,
        IARG_END);

    if (INS_IsMemoryRead(ins))
    {
        const UINT32 size = INS_MemoryReadSize(ins);
        const AFUNPTR countFun = (size <= 4 ? (AFUNPTR) MemRefSingle : (AFUNPTR) MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, countFun,
            IARG_MEMORYREAD_EA,
            IARG_MEMORYREAD_SIZE,
            IARG_UINT32, 'l',
            IARG_END);
    }

    if (INS_IsMemoryWrite(ins))
    {
        const UINT32 size = INS_MemoryWriteSize(ins);
        const AFUNPTR countFun = (size <= 4 ? (AFUNPTR) MemRefSingle : (AFUNPTR) MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, countFun,
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_UINT32, 's',
            IARG_END);
    }
}

GLOBALFUN int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);

    // Create instruction cache.
    icache = create_cache_struct(icache_size.Value(),
				 icache_associativity.Value(),
				 icache_block_size.Value());

    if (icache == NULL)
    {
	printf("Unable to create an instruction cache with the following parameters:\n");
	printf("Size (bytes): %d\n", icache_size.Value());
	printf("Associativity: %d\n", icache_associativity.Value());
	printf("Block size: %d\n", icache_block_size.Value());
	fflush(stdout);
	exit(-1);
    }

    // Create data cache.
    dcache = create_cache_struct(dcache_size.Value(),
				 dcache_associativity.Value(),
				 dcache_block_size.Value());

    if (dcache == NULL)
    {
	printf("Unable to create a data cache with the following parameters:\n");
	printf("Size (bytes): %d\n", dcache_size.Value());
	printf("Associativity: %d\n", dcache_associativity.Value());
	printf("Block size: %d\n", dcache_block_size.Value());
	fflush(stdout);
	exit(-1);
    }

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0; // make compiler happy
}
