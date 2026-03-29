typedef struct {
    int cache_size;
    int block_size;
    int associativity;
    char* replacement_policy;
    int physical_memory;
    float physical_memory_usage_percentage;
    float instructions_per_timeslice;
    char* trace_files[3];
} SimConfig;
