#ifndef SIM_H
#define SIM_H

#include <stdio.h>

#define MAX_TRC_FILES 3
#define PAGE_SIZE 4096U
#define PAGE_TABLE_ENTRIES 524288U

typedef struct {
    int cache_size;
    int block_size;
    int associativity;
    char *replacement_policy;
    int physical_memory;
    float physical_memory_usage_percentage;
    int instructions_per_timeslice;
    char *trace_files[MAX_TRC_FILES];
    int num_trace_files;
} SimConfig;

typedef struct {
    int index_bits;
    int tag_bits;
    int total_rows;
    int total_blocks;
    int overhead_bytes;
    int implementation_bytes;
    int overhead_per_block_bits;
    double overhead_per_block_bytes;
    float cost;
} CacheCalc;

typedef struct {
    long num_physical_pages;
    long num_system_pages;
    long num_user_pages;
    int pte_bits;
    int page_table_bytes;
    int per_process_page_table_bytes;
} PhysicalCalc;

typedef struct {
    int valid;
    unsigned int physical_page;
} PageTableEntry;

typedef struct {
    FILE *file;
    char *name;
    int done;
    long used_entries;
    PageTableEntry *page_table;
} TraceProcess;

typedef struct {
    int owner_process;
    unsigned int owner_virtual_page;
    int allocated;
} PhysicalPageFrame;

typedef struct {
    long virtual_pages_mapped;
    long page_table_hits;
    long pages_from_free;
    long total_page_faults;
} VmStats;

typedef struct {
    int valid;
    unsigned int tag;
    unsigned int physical_page;
} CacheLine;

typedef struct {
    CacheLine *lines;
    int *rr_next_way;
    int total_rows;
    int associativity;
    int block_size;
    int offset_bits;
    int index_bits;
    int total_blocks;
} Cache;

typedef struct {
    long total_cache_accesses;
    long total_addresses;
    long instruction_bytes;
    long srcdst_bytes;
    long cache_hits;
    long cache_misses;
    long compulsory_misses;
    long conflict_misses;
    long total_instructions;
    long long total_cycles;
} CacheStats;

#endif
