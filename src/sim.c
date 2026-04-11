#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sim.h"

static void fail(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static int is_power_of_two(int value) {
    return value > 0 && (value & (value - 1)) == 0;
}

static unsigned int extract_virtual_page(unsigned int address) {
    return address / PAGE_SIZE;
}

static SimConfig *read_args(int argc, char *argv[]) {
    SimConfig *sim_config = malloc(sizeof(SimConfig));
    int saw_s = 0;
    int saw_b = 0;
    int saw_a = 0;
    int saw_r = 0;
    int saw_p = 0;
    int saw_u = 0;
    int saw_n = 0;

    if (sim_config == NULL) {
        fail("Error: Memory allocation failed.");
    }

    sim_config->cache_size = 8;
    sim_config->block_size = 8;
    sim_config->associativity = 1;
    sim_config->replacement_policy = "rr";
    sim_config->physical_memory = 128;
    sim_config->physical_memory_usage_percentage = 0.0f;
    sim_config->instructions_per_timeslice = -1;
    sim_config->num_trace_files = 0;

    for (int i = 0; i < MAX_TRC_FILES; i++) {
        sim_config->trace_files[i] = NULL;
    }

    for (int i = 1; i < argc; i++) {
        char *endptr = NULL;
        int temp = 0;

        if (argv[i][0] != '-') {
            continue;
        }

        if (i + 1 >= argc) {
            fprintf(stderr, "Error: Missing value for flag %s.\n", argv[i]);
            exit(1);
        }

        switch (argv[i][1]) {
            case 's':
                temp = (int)strtol(argv[++i], &endptr, 10);
                if (*endptr != '\0' || temp < 8 || temp > 8192 || !is_power_of_two(temp)) {
                    fprintf(stderr, "Error: Invalid cache size %s.\n", argv[i]);
                    exit(1);
                }
                sim_config->cache_size = temp;
                saw_s = 1;
                break;

            case 'b':
                temp = (int)strtol(argv[++i], &endptr, 10);
                if (*endptr != '\0' || temp < 8 || temp > 64 || !is_power_of_two(temp)) {
                    fprintf(stderr, "Error: Invalid block size %s.\n", argv[i]);
                    exit(1);
                }
                sim_config->block_size = temp;
                saw_b = 1;
                break;

            case 'a':
                temp = (int)strtol(argv[++i], &endptr, 10);
                if (*endptr != '\0' || temp < 1 || temp > 16 || !is_power_of_two(temp)) {
                    fprintf(stderr, "Error: Invalid associativity %s.\n", argv[i]);
                    exit(1);
                }
                sim_config->associativity = temp;
                saw_a = 1;
                break;

            case 'r': {
                char policy[8] = {0};
                int policy_index = 0;

                for (const char *p = argv[++i]; *p != '\0' && policy_index < 7; p++) {
                    policy[policy_index++] = (char)tolower((unsigned char)*p);
                }
                policy[policy_index] = '\0';

                if (strcmp(policy, "rr") != 0 && strcmp(policy, "rnd") != 0) {
                    fprintf(stderr, "Error: Invalid replacement policy %s.\n", argv[i]);
                    exit(1);
                }
                sim_config->replacement_policy = strcmp(policy, "rr") == 0 ? "rr" : "rnd";
                saw_r = 1;
                break;
            }

            case 'p':
                temp = (int)strtol(argv[++i], &endptr, 10);
                if (*endptr != '\0' || temp < 128 || temp > 4096 || !is_power_of_two(temp)) {
                    fprintf(stderr, "Error: Invalid physical memory %s.\n", argv[i]);
                    exit(1);
                }
                sim_config->physical_memory = temp;
                saw_p = 1;
                break;

            case 'u':
                sim_config->physical_memory_usage_percentage = strtof(argv[++i], &endptr);
                if (*endptr != '\0' ||
                    sim_config->physical_memory_usage_percentage < 0.0f ||
                    sim_config->physical_memory_usage_percentage > 100.0f) {
                    fprintf(stderr, "Error: Invalid system memory percentage %s.\n", argv[i]);
                    exit(1);
                }
                saw_u = 1;
                break;

            case 'n':
                temp = (int)strtol(argv[++i], &endptr, 10);
                if (*endptr != '\0' || temp == 0 || temp < -1) {
                    fprintf(stderr, "Error: Invalid instructions per time slice %s.\n", argv[i]);
                    exit(1);
                }
                sim_config->instructions_per_timeslice = temp;
                saw_n = 1;
                break;

            case 'f':
                if (sim_config->num_trace_files >= MAX_TRC_FILES) {
                    fail("Error: A maximum of 3 trace files is allowed.");
                }
                sim_config->trace_files[sim_config->num_trace_files++] = argv[++i];
                break;

            default:
                fprintf(stderr, "Error: Invalid flag %s.\n", argv[i]);
                exit(1);
        }
    }

    if (!saw_s || !saw_b || !saw_a || !saw_r || !saw_p || !saw_u || !saw_n || sim_config->num_trace_files == 0) {
        fail("Invalid usage. Required flags: -s -b -a -r -p -u -n and at least one -f.");
    }

    return sim_config;
}

static CacheCalc *calculate_cache(const SimConfig *sim_config) {
    CacheCalc *output = malloc(sizeof(CacheCalc));
    int total_blocks;
    int total_rows;
    int offset_bits;
    int index_bits;
    int physical_address_bits;
    int tag_bits;

    if (output == NULL) {
        fail("Error: Memory allocation failed.");
    }

    total_blocks = (sim_config->cache_size * 1024) / sim_config->block_size;
    total_rows = total_blocks / sim_config->associativity;
    offset_bits = (int)log2((double)sim_config->block_size);
    index_bits = (int)log2((double)total_rows);
    physical_address_bits = (int)log2((double)((long long)sim_config->physical_memory * 1024 * 1024));
    tag_bits = physical_address_bits - index_bits - offset_bits;

    output->total_blocks = total_blocks;
    output->total_rows = total_rows;
    output->tag_bits = tag_bits;
    output->index_bits = index_bits;
    output->overhead_bytes = (int)ceil((double)(tag_bits + 1) * total_blocks / 8.0);
    output->implementation_bytes = (sim_config->cache_size * 1024) + output->overhead_bytes;
    output->cost = (output->implementation_bytes / 1024.0f) * 0.07f;

    return output;
}

static PhysicalCalc *calculate_physical(const SimConfig *sim_config) {
    PhysicalCalc *output = malloc(sizeof(PhysicalCalc));
    long num_physical_pages;
    long num_system_pages;
    int physical_page_bits;

    if (output == NULL) {
        fail("Error: Memory allocation failed.");
    }

    num_physical_pages = ((long)sim_config->physical_memory * 1024L * 1024L) / PAGE_SIZE;
    num_system_pages = (long)((sim_config->physical_memory_usage_percentage / 100.0f) * num_physical_pages);
    if (num_system_pages > num_physical_pages) {
        num_system_pages = num_physical_pages;
    }

    physical_page_bits = (int)log2((double)num_physical_pages);

    output->num_physical_pages = num_physical_pages;
    output->num_system_pages = num_system_pages;
    output->num_user_pages = num_physical_pages - num_system_pages;
    output->pte_bits = 1 + physical_page_bits;
    output->page_table_bytes = (int)(((long long)PAGE_TABLE_ENTRIES * sim_config->num_trace_files * output->pte_bits) / 8);
    output->per_process_page_table_bytes = (int)(((long long)PAGE_TABLE_ENTRIES * output->pte_bits) / 8);

    return output;
}

static void print_milestone1(const SimConfig *sim_config, const CacheCalc *cache_calc, const PhysicalCalc *physical_calc) {
    printf("Cache Simulator - CS 3853 - Team #01\n\n");

    printf("Trace File(s):\n");
    for (int i = 0; i < sim_config->num_trace_files; i++) {
        printf("        %s\n", sim_config->trace_files[i]);
    }

    printf("\n***** Cache Input Parameters *****\n\n");
    printf("%-30s  %d KB\n", "Cache Size:", sim_config->cache_size);
    printf("%-30s  %d bytes\n", "Block Size:", sim_config->block_size);
    printf("%-30s  %d\n", "Associativity:", sim_config->associativity);
    printf("%-30s  %s\n", "Replacement Policy:", strcmp(sim_config->replacement_policy, "rr") == 0 ? "Round Robin" : "Random");
    printf("%-30s  %d MB\n", "Physical Memory:", sim_config->physical_memory);
    printf("%-30s  %.1f%%\n", "Percent Memory Used by System:", sim_config->physical_memory_usage_percentage);
    if (sim_config->instructions_per_timeslice == -1) {
        printf("%-30s  max\n", "Instructions / Time Slice:");
    } else {
        printf("%-30s  %d\n", "Instructions / Time Slice:", sim_config->instructions_per_timeslice);
    }

    printf("\n***** Cache Calculated Values *****\n\n");
    printf("%-30s  %d\n", "Total # Blocks:", cache_calc->total_blocks);
    printf("%-30s  %d bits\n", "Tag Size:", cache_calc->tag_bits);
    printf("%-30s  %d bits\n", "Index Size:", cache_calc->index_bits);
    printf("%-30s  %d\n", "Total # Rows:", cache_calc->total_rows);
    printf("%-30s  %d bytes\n", "Overhead Size:", cache_calc->overhead_bytes);
    printf("%-30s  %.2f KB  (%d bytes)\n", "Implementation Memory Size:",
           cache_calc->implementation_bytes / 1024.0, cache_calc->implementation_bytes);
    printf("%-30s  $%.2f @ $0.07 per KB\n", "Cost:", cache_calc->cost);

    printf("\n***** Physical Memory Calculated Values *****\n\n");
    printf("%-30s  %ld\n", "Number of Physical Pages:", physical_calc->num_physical_pages);
    printf("%-30s  %ld\n", "Number of Pages for System:", physical_calc->num_system_pages);
    printf("%-30s  %d bits\n", "Size of Page Table Entry:", physical_calc->pte_bits);
    printf("%-30s  %d bytes\n", "Total RAM for Page Table(s):", physical_calc->page_table_bytes);
}

static TraceProcess *init_processes(const SimConfig *sim_config) {
    TraceProcess *processes = calloc((size_t)sim_config->num_trace_files, sizeof(TraceProcess));

    if (processes == NULL) {
        fail("Error: Memory allocation failed.");
    }

    for (int i = 0; i < sim_config->num_trace_files; i++) {
        processes[i].file = fopen(sim_config->trace_files[i], "r");
        if (processes[i].file == NULL) {
            fprintf(stderr, "Error: Unable to open trace file %s.\n", sim_config->trace_files[i]);
            exit(1);
        }

        processes[i].name = sim_config->trace_files[i];
        processes[i].done = 0;
        processes[i].used_entries = 0;
        processes[i].page_table = calloc(PAGE_TABLE_ENTRIES, sizeof(PageTableEntry));
        if (processes[i].page_table == NULL) {
            fail("Error: Memory allocation failed.");
        }
    }

    return processes;
}

static void free_processes(TraceProcess *processes, int count) {
    if (processes == NULL) {
        return;
    }

    for (int i = 0; i < count; i++) {
        if (processes[i].file != NULL) {
            fclose(processes[i].file);
        }
        free(processes[i].page_table);
    }

    free(processes);
}

static int parse_instruction_line(const char *line, unsigned int *address) {
    unsigned int length = 0;

    if (sscanf(line, "EIP (%u): %x", &length, address) == 2) {
        return 1;
    }

    return 0;
}

static void parse_data_line(const char *line, unsigned int *dst, int *dst_valid, unsigned int *src, int *src_valid) {
    char dst_addr[9] = {0};
    char dst_data[9] = {0};
    char src_addr[9] = {0};
    char src_data[9] = {0};

    *dst = 0;
    *src = 0;
    *dst_valid = 0;
    *src_valid = 0;

    if (sscanf(line, "dstM: %8s %8s srcM: %8s %8s", dst_addr, dst_data, src_addr, src_data) != 4) {
        return;
    }

    if (strcmp(dst_data, "--------") != 0 && strcmp(dst_addr, "00000000") != 0) {
        *dst = (unsigned int)strtoul(dst_addr, NULL, 16);
        *dst_valid = 1;
    }

    if (strcmp(src_data, "--------") != 0 && strcmp(src_addr, "00000000") != 0) {
        *src = (unsigned int)strtoul(src_addr, NULL, 16);
        *src_valid = 1;
    }
}

static void map_virtual_page(TraceProcess *processes,
                             int process_index,
                             unsigned int virtual_page,
                             PhysicalPageFrame *frames,
                             long user_page_count,
                             long *next_free_frame,
                             long *replacement_cursor,
                             const char *replacement_policy,
                             VmStats *stats) {
    PageTableEntry *entry = &processes[process_index].page_table[virtual_page];
    long frame_index;

    stats->virtual_pages_mapped++;

    if (entry->valid) {
        stats->page_table_hits++;
        return;
    }

    if (*next_free_frame < user_page_count) {
        frame_index = *next_free_frame;
        (*next_free_frame)++;
        stats->pages_from_free++;
    } else {
        stats->total_page_faults++;
        if (strcmp(replacement_policy, "rnd") == 0) {
            frame_index = rand() % user_page_count;
        } else {
            frame_index = *replacement_cursor;
            *replacement_cursor = (*replacement_cursor + 1) % user_page_count;
        }

        if (frames[frame_index].owner_process >= 0) {
            TraceProcess *victim_process = &processes[frames[frame_index].owner_process];
            PageTableEntry *victim_entry = &victim_process->page_table[frames[frame_index].owner_virtual_page];
            victim_entry->valid = 0;
            if (victim_process->used_entries > 0) {
                victim_process->used_entries--;
            }
        }
    }

    entry->valid = 1;
    entry->physical_page = (unsigned int)frame_index;
    processes[process_index].used_entries++;
    frames[frame_index].owner_process = process_index;
    frames[frame_index].owner_virtual_page = virtual_page;
}

static int process_trace_instruction(TraceProcess *processes,
                                     int process_index,
                                     PhysicalPageFrame *frames,
                                     long user_page_count,
                                     long *next_free_frame,
                                     long *replacement_cursor,
                                     const char *replacement_policy,
                                     VmStats *stats) {
    char instruction_line[512];
    char data_line[512];
    unsigned int instruction_address = 0;
    unsigned int dst_address = 0;
    unsigned int src_address = 0;
    int dst_valid = 0;
    int src_valid = 0;

    while (fgets(instruction_line, sizeof(instruction_line), processes[process_index].file) != NULL) {
        if (!parse_instruction_line(instruction_line, &instruction_address)) {
            continue;
        }

        if (fgets(data_line, sizeof(data_line), processes[process_index].file) == NULL) {
            processes[process_index].done = 1;
            return 0;
        }

        parse_data_line(data_line, &dst_address, &dst_valid, &src_address, &src_valid);

        map_virtual_page(processes, process_index, extract_virtual_page(instruction_address),
                         frames, user_page_count, next_free_frame, replacement_cursor,
                         replacement_policy, stats);

        if (dst_valid) {
            map_virtual_page(processes, process_index, extract_virtual_page(dst_address),
                             frames, user_page_count, next_free_frame, replacement_cursor,
                             replacement_policy, stats);
        }

        if (src_valid) {
            map_virtual_page(processes, process_index, extract_virtual_page(src_address),
                             frames, user_page_count, next_free_frame, replacement_cursor,
                             replacement_policy, stats);
        }

        return 1;
    }

    processes[process_index].done = 1;
    return 0;
}

static VmStats run_virtual_memory(const SimConfig *sim_config,
                                  const PhysicalCalc *physical_calc,
                                  TraceProcess *processes) {
    VmStats stats = {0, 0, 0, 0};
    PhysicalPageFrame *frames = NULL;
    long next_free_frame = 0;
    long replacement_cursor = 0;
    int active_processes = sim_config->num_trace_files;

    if (physical_calc->num_user_pages <= 0) {
        fail("Error: No physical pages available to user processes.");
    }

    frames = malloc((size_t)physical_calc->num_user_pages * sizeof(PhysicalPageFrame));
    if (frames == NULL) {
        fail("Error: Memory allocation failed.");
    }

    for (long i = 0; i < physical_calc->num_user_pages; i++) {
        frames[i].owner_process = -1;
        frames[i].owner_virtual_page = 0;
    }

    while (active_processes > 0) {
        for (int i = 0; i < sim_config->num_trace_files; i++) {
            int instructions_this_slice = 0;
            int limit = sim_config->instructions_per_timeslice;

            if (processes[i].done) {
                continue;
            }

            while (limit == -1 || instructions_this_slice < limit) {
                if (!process_trace_instruction(processes, i, frames, physical_calc->num_user_pages,
                                               &next_free_frame, &replacement_cursor,
                                               sim_config->replacement_policy, &stats)) {
                    active_processes--;
                    break;
                }
                instructions_this_slice++;
            }
        }
    }

    free(frames);
    return stats;
}

static void print_milestone2(const SimConfig *sim_config,
                             const PhysicalCalc *physical_calc,
                             const TraceProcess *processes,
                             const VmStats *stats) {
    printf("\n\nMILESTONE #2: - Virtual Memory Simulation Results\n\n");
    printf("***** VIRTUAL MEMORY SIMULATION RESULTS *****\n\n");
    printf("%-30s  %ld\n", "Physical Pages Used By SYSTEM:", physical_calc->num_system_pages);
    printf("%-30s  %ld\n\n", "Pages Available to User:", physical_calc->num_user_pages);

    printf("%-30s  %ld\n", "Virtual Pages Mapped:", stats->virtual_pages_mapped);
    printf("        ------------------------------\n");
    printf("%-30s  %ld\n", "        Page Table Hits:", stats->page_table_hits);
    printf("%-30s  %ld\n", "        Pages from Free:", stats->pages_from_free);
    printf("%-30s  %ld\n\n", "        Total Page Faults:", stats->total_page_faults);

    printf("Page Table Usage Per Process:\n");
    printf("------------------------------\n");

    for (int i = 0; i < sim_config->num_trace_files; i++) {
        int used_bytes = (int)ceil((double)processes[i].used_entries * physical_calc->pte_bits / 8.0);
        int wasted_bytes = physical_calc->per_process_page_table_bytes - used_bytes;
        double percent = (processes[i].used_entries * 100.0) / PAGE_TABLE_ENTRIES;

        printf("[%d] %s:\n", i, processes[i].name);
        printf("        Used Page Table Entries: %ld  (%.2f%%)\n", processes[i].used_entries, percent);
        printf("        Page Table Wasted: %d bytes\n\n", wasted_bytes);
    }
}

int main(int argc, char *argv[]) {
    SimConfig *sim_config;
    CacheCalc *cache_calc;
    PhysicalCalc *physical_calc;
    TraceProcess *processes;
    VmStats vm_stats;

    srand((unsigned int)time(NULL));

    sim_config = read_args(argc, argv);
    cache_calc = calculate_cache(sim_config);
    physical_calc = calculate_physical(sim_config);
    processes = init_processes(sim_config);

    print_milestone1(sim_config, cache_calc, physical_calc);
    vm_stats = run_virtual_memory(sim_config, physical_calc, processes);
    print_milestone2(sim_config, physical_calc, processes, &vm_stats);

    free_processes(processes, sim_config->num_trace_files);
    free(sim_config);
    free(cache_calc);
    free(physical_calc);

    return 0;
}
