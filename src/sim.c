#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"

#define MIN_ARGS 17

//Group 1
//Mateo McKee, Elian Garcia, Chinedum Akunne, Ayden Trevino

SimConfig* read_args(int argc, char* argv[]) {
    SimConfig* config = (SimConfig*)malloc(sizeof(SimConfig));

    int temp = 0;
    char* endptr;

    //cache size
    temp = (int)strtol(argv[2], &endptr, 10); 
    if(*endptr != '\0' || temp < 8 || temp > 8192 || ((temp & (temp-1)) != 0)) {
        fprintf(stderr, "Error: Invalid cache size %s. Must be a power of 2 integer between 8 and 8192.\n", argv[2]);
        exit(1);
    }
    config->cache_size = temp;

    //block size
    temp = (int)strtol(argv[4], &endptr, 10); 
    if(*endptr != '\0' || temp < 8 || temp > 64 || ((temp & (temp-1)) != 0)) {
        fprintf(stderr, "Error: Invalid block size %s. Must be a power of 2 integer between 8 and 64.\n", argv[4]);
        exit(1);
    }
    config->block_size = temp;

    //associativity
    temp = (int)strtol(argv[6], &endptr, 10); 
    if(*endptr != '\0' || temp > 16 || temp < 1 || ((temp & (temp-1)) != 0)) {
        fprintf(stderr, "Error: Invalid associativity %s. Must be either 1, 2, 4, 8, 16.\n", argv[6]);
        exit(1);
    }
    config->associativity = temp;

    //replacement policy
    if(strcmp(argv[8], "RR") != 0 && strcmp(argv[8], "RND") != 0) {
        fprintf(stderr, "Error: Invalid replacement policy %s. Must be either RR or RND.\n", argv[8]);
        exit(1);
    }
    config->replacement_policy = argv[8];

    //physical memory
    temp = (int)strtol(argv[10], &endptr, 10); 
    if(*endptr != '\0' || temp < 128 || temp > 4000 || ((temp & (temp-1)) != 0)) {
        fprintf(stderr, "Error: Invalid physical memory %s. Must be a power of 2 integer between 128 and 4096.\n", argv[10]);
        exit(1);
    }
    config->physical_memory = temp;

    //physical memory usage percentage
    temp = (int)strtol(argv[12], &endptr, 10); 
    if(*endptr != '\0' || temp < 0 || temp > 100) {
        fprintf(stderr, "Error: Invalid physical memory usage percentage %s. Must be an integer between 0 and 100.\n", argv[12]);
        exit(1);
    }
    config->physical_memory_usage_percentage = temp;

    //instruction/timeslice
    float f_temp = strtof(argv[14], &endptr); 
    if(*endptr != '\0' || f_temp < 1 || f_temp > 100) {
        fprintf(stderr, "Error: Invalid instructions per timeslice %s. Must be a float between 0 and 100.\n", argv[14]);
        exit(1);
    }
    config->instructions_per_timeslice = f_temp;

    //trace files (min 1, max 3)
    int num_files = (argc - MIN_ARGS)/2 + 1;
  
    int i;
    for(i = 0; i < num_files; i++) {
        *(config->trace_files+i) = argv[16+(2*i)];
    }

    return config;
}

int main(int argc, char* argv[]) {
    // arg check
    if(argc < MIN_ARGS) {
    
        fprintf(stderr, "Invalid usage. Please format as follows:\n ./sim.o -s <cache size in KB> -b <block size> -a <associativity> -r <replacement policy> -p <physical memory in MB> -u <%% physical mem used by OS> -n <instructions/time slice> -f <trace_file>\n");
        return 1;
    }

    SimConfig config = *read_args(argc, argv);

    printf("Given config: cache size %d KB, block size %d B, %d-way associativity, %s replacement policy, physical memory %d MB, physical memory usage %f%%, instructions per timeslice %f, tracefile1 %s, tracefile2 %s, tracefile3 %s\n",
            config.cache_size, config.block_size, config.associativity, config.replacement_policy, config.physical_memory, config.physical_memory_usage_percentage, config.instructions_per_timeslice, config.trace_files[0], config.trace_files[1], config.trace_files[2]);
    
    return 0;
}
