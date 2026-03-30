#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sim.h"
#include <ctype.h>

#define MIN_ARGS 17

//Group 1
//Mateo McKee, Elian Garcia, Chinedum Akunne, Ayden Trevino

SimConfig* read_args(int argc, char* argv[]) {
    SimConfig* sim_config = (SimConfig*)malloc(sizeof(SimConfig));

    int temp = 0;
    char* endptr;

    //cache size
    temp = (int)strtol(argv[2], &endptr, 10); 
    if(*endptr != '\0' || temp < 8 || temp > 8192 || ((temp & (temp-1)) != 0)) {
        fprintf(stderr, "Error: Invalid cache size %s. Must be a power of 2 integer between 8 and 8192.\n", argv[2]);
        exit(1);
    }
    sim_config->cache_size = temp;

    //block size
    temp = (int)strtol(argv[4], &endptr, 10); 
    if(*endptr != '\0' || temp < 8 || temp > 64 || ((temp & (temp-1)) != 0)) {
        fprintf(stderr, "Error: Invalid block size %s. Must be a power of 2 integer between 8 and 64.\n", argv[4]);
        exit(1);
    }
    sim_config->block_size = temp;

    //associativity
    temp = (int)strtol(argv[6], &endptr, 10); 
    if(*endptr != '\0' || temp > 16 || temp < 1 || ((temp & (temp-1)) != 0)) {
        fprintf(stderr, "Error: Invalid associativity %s. Must be either 1, 2, 4, 8, 16.\n", argv[6]);
        exit(1);
    }
    sim_config->associativity = temp;

    //replacement policy
    if(strcmp(argv[8], "rr") != 0 && strcmp(argv[8], "rnd") != 0) {
        fprintf(stderr, "Error: Invalid replacement policy \"%s\". Must be either \"rr\" or \"rnd\".\n", argv[8]);
        exit(1);
    }
    sim_config->replacement_policy = argv[8];

    //physical memory
    temp = (int)strtol(argv[10], &endptr, 10); 
    if(*endptr != '\0' || temp < 128 || temp > 4000 || ((temp & (temp-1)) != 0)) {
        fprintf(stderr, "Error: Invalid physical memory %s. Must be a power of 2 integer between 128 and 4096.\n", argv[10]);
        exit(1);
    }
    sim_config->physical_memory = temp;

    //physical memory usage percentage
    temp = (int)strtol(argv[12], &endptr, 10); 
    if(*endptr != '\0' || temp < 0 || temp > 100) {
        fprintf(stderr, "Error: Invalid physical memory usage percentage %s. Must be an integer between 0 and 100.\n", argv[12]);
        exit(1);
    }
    sim_config->physical_memory_usage_percentage = temp;

    //instruction/timeslice
    float f_temp = strtof(argv[14], &endptr); 
    if(*endptr != '\0' || f_temp < 1 || f_temp > 100) {
        fprintf(stderr, "Error: Invalid instructions per timeslice %s. Must be a float between 0 and 100.\n", argv[14]);
        exit(1);
    }
    sim_config->instructions_per_timeslice = f_temp;

    //trace files (min 1, max 3)
    int num_trace_files = (argc - MIN_ARGS)/2 + 1;
    
    //cap num of trace files to max amount in order to control user input
    if(num_trace_files > MAX_TRC_FILES) {
        num_trace_files = MAX_TRC_FILES;
    }

    sim_config->num_trace_files = num_trace_files;

    int i;
    for(i = 0; i < num_trace_files; i++) {
        *(sim_config->trace_files+i) = argv[16+(2*i)];
    }

    return sim_config;
}

CacheCalc* calculate_cache(SimConfig* sim_config) {

    // cache calculated values
    int total_blocks = (sim_config->cache_size * 1024) / sim_config->block_size;
    int total_rows = total_blocks / sim_config->associativity;

    int offset_bits = (int)log2(sim_config->block_size);
    int index_bits = (int)log2(total_rows);

    // tag bits based on physical memory
    int physical_address_bits = (int)log2((long)sim_config->physical_memory * 1024 * 1024);
    int tag_bits = physical_address_bits - index_bits - offset_bits;

    // overhead: (tag_bits + 1 valid bit) * total_blocks / 8 bytes
    int overhead_bytes = (int)ceil((double)(tag_bits + 1) * total_blocks / 8.0);

    int implementation_bytes = (sim_config->cache_size * 1024) + overhead_bytes;
    float cost = (implementation_bytes / 1024.0) * 0.07;

    CacheCalc* output = (CacheCalc*)malloc(sizeof(CacheCalc));
    output->total_blocks = total_blocks;
    output->total_rows = total_rows;
    output->tag_bits = tag_bits;
    output->index_bits = index_bits;
    output->overhead_bytes = overhead_bytes;
    output->implementation_bytes = implementation_bytes;
    output->cost = cost;

    return output;
}

PhysicalCalc* calculate_physical(SimConfig* sim_config) {
    int num_physical_pages = sim_config->physical_memory * 1024 * 1024 / 4096;
    int num_system_pages = sim_config->physical_memory_usage_percentage / 100.0 * num_physical_pages;

    int physical_page_bits = log2(num_physical_pages);

    int pte_bits = 1 + physical_page_bits;
    int page_table_bytes = (int)(long)524288 * sim_config->num_trace_files * pte_bits / 8;

    PhysicalCalc* output = (PhysicalCalc*)malloc(sizeof(PhysicalCalc));
    output->num_physical_pages = num_physical_pages;
    output->num_system_pages = num_system_pages;
    output->pte_bits = pte_bits;
    output->page_table_bytes = page_table_bytes;

    return output;
}

// print method AI generated because no way im formatting all that
void print_milestone1(SimConfig* sim_config, CacheCalc* cache_calc, PhysicalCalc* physical_calc) {
    printf("CPU Simulator - CS 3853 - Team #01\n\n");

    printf("Trace File(s):\n");
    for (int i = 0; i < sim_config->num_trace_files; i++)
        if (sim_config->trace_files[i] != NULL)
            printf("\t%s\n", sim_config->trace_files[i]);

    printf("\n***** Cache Input Parameters *****\n\n");
    printf("%-30s %d KB\n",    "Cache Size:",                    sim_config->cache_size);
    printf("%-30s %d bytes\n", "Block Size:",                    sim_config->block_size);
    printf("%-30s %d\n",       "Associativity:",                 sim_config->associativity);
    printf("%-30s %s\n",       "Replacement Policy:",            sim_config->replacement_policy);
    printf("%-30s %d MB\n",    "Physical Memory:",               sim_config->physical_memory);
    printf("%-30s %.1f%%\n",   "Percent Memory Used by System:", sim_config->physical_memory_usage_percentage);
    printf("%-30s %.0f\n",     "Instructions / Time Slice:",     sim_config->instructions_per_timeslice);

    printf("\n***** Cache Calculated Values *****\n\n");
    printf("%-30s %d\n",          "Total # Blocks:",              cache_calc->total_blocks);
    printf("%-30s %d bits\n",     "Tag Size:",                    cache_calc->tag_bits);
    printf("%-30s %d bits\n",     "Index Size:",                  cache_calc->index_bits);
    printf("%-30s %d\n",          "Total # Rows:",                cache_calc->total_rows);
    printf("%-30s %d bytes\n",    "Overhead Size:",               cache_calc->overhead_bytes);
    printf("%-30s %.2f KB  (%d bytes)\n", "Implementation Memory Size:",
           cache_calc->implementation_bytes / 1024.0, cache_calc->implementation_bytes);
    printf("%-30s $%.2f @ $0.07 per KB\n", "Cost:",              cache_calc->cost);

    printf("\n***** Physical Memory Calculated Values *****\n\n");
    printf("%-30s %d\n",       "Number of Physical Pages:",       physical_calc->num_physical_pages);
    printf("%-30s %d\n",       "Number of Pages for System:",     physical_calc->num_system_pages);
    printf("%-30s %d bits\n",  "Size of Page Table Entry:",       physical_calc->pte_bits);
    printf("%-30s %d bytes\n", "Total RAM for Page Table(s):",    physical_calc->page_table_bytes);
}


int main(int argc, char* argv[]) {
    // arg check
    if(argc < MIN_ARGS) {
        fprintf(stderr, "Invalid usage. Please format as follows:\n ./sim.o -s <cache size in KB> -b <block size> -a <associativity> -r <replacement policy> -p <physical memory in MB> -u <%% physical mem used by OS> -n <instructions/time slice> -f <trace_file>\n");
        return 1;
    }

    SimConfig* sim_config = read_args(argc, argv);

    CacheCalc* cache_calc = calculate_cache(sim_config);
    PhysicalCalc* physical_calc = calculate_physical(sim_config);

    print_milestone1(sim_config, cache_calc, physical_calc);

    free(sim_config);
    free(cache_calc);
    free(physical_calc);

    return 0;
}
