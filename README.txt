mini-cpu

Milestone #2 is implemented at a basic level in `src/sim.c`.

Build:
`make`

Run:
`./sim.o -s <cache_kb> -b <block_bytes> -a <assoc> -r <rr|rnd> -p <phys_mem_mb> -u <os_percent> -n <timeslice|-1> -f <trace1> [-f <trace2>] [-f <trace3>]`

Current scope:
- Keeps Milestone #1 header/calculation output.
- Adds virtual memory simulation for 1 to 3 trace files.
- Uses 4 KB pages and one page table per process.
- Supports free-page allocation and replacement on page faults.
- Prints Milestone #2 virtual memory stats and per-process page table usage.

Not included:
- Milestone #3 cache simulation.
