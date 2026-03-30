#define MAX_TRC_FILES 3

typedef struct {
    int cache_size;
    int block_size;
    int associativity;
    char* replacement_policy;
    int physical_memory;
    float physical_memory_usage_percentage;
    float instructions_per_timeslice;
    char* trace_files[MAX_TRC_FILES];
    int num_trace_files;
} SimConfig;

typedef struct {
    int index_bits;
    int tag_bits;
    int total_rows;
    int total_blocks;
    int overhead_bytes; 
    int implementation_bytes; 
    float cost;
} CacheCalc;

typedef struct {
    int num_physical_pages;
    int num_system_pages;
    int pte_bits;
    int page_table_bytes;
} PhysicalCalc;
