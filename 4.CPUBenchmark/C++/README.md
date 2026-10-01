# Implementation

Besides all the organizational code, I'll put focus onto the src/squarematrix.cpp file that contains all the interesting code. Here is the list of all optimizations that lead to a meaningful boost in performance:
- Alignas(64) - Makes sure that all reads and writes are cacheline aligned and allows to use aligned AVX2 calls.
- Packaging - Allowed to further optimize the micro-kernel by giving it a fixed, known size for the tile.
- Tiling - Ensures cache locality to stop wasting time on LLC load misses. The size of the tile was chosen manually based on best performance.
- Loop reordering inside the micro-kernel - Uses accumulators to keep values inside registers longer. 
- 3x3 kernel - The CPU has 16 registers, and this size utilises 15 out of 16.
- Manual unrolling - The micro-kernel is manually unrolled to ensure instruction parallelism and that no more registers than available are used.
- Prefetching - Makes sure that the next blocks are preloaded during stripes.
- Sequential access - All data retrieval is made as parallel as possible.
- Locality - All operations are placed as close together as possible to ensure that the TLB is not trashed.

# Measurements against BLAS

Both measurements were taken on an Intel i5-9500f without pinned cores, using this command:
```
perf stat -r 1 -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,LLC-loads,LLC-load-misses,L1-dcache-loads,L1-dcache-load-misses,dTLB-load-misses,dTLB-store-misses,iTLB-load-misses ./foxbench -c 6 -m128MiB -i 100
```

Fox Bench™ results:
```
Average speed after 100 iterations and matrix size of 1200: 7.60279e+10 flops/s
Max value: 8.13101e+10 flops/s

Performance counter stats for './foxbench -c 6 -m128MiB -i 100':

    18 982 406 383      cycles:u
    51 490 350 087      instructions:u
     2 738 926 624      branches:u
        26 472 965      branch-misses:u
     1 548 075 981      cache-references:u
       252 034 641      cache-misses:u
       224 702 035      LLC-loads:u
        85 865 380      LLC-load-misses:u
    16 179 247 738      L1-dcache-loads:u
     4 696 405 907      L1-dcache-load-misses:u
           422 674      dTLB-load-misses:u
           239 631      dTLB-store-misses:u
            10 271      iTLB-load-misses:u

       0,866311997 seconds time elapsed

       4,501730000 seconds user
       0,250534000 seconds sys
```

BLAS results (taken by simply replacing SquareMatrix::multiplyAdd body with single call to
cblass_sgemm function.):
```
Average speed after 100 iterations and matrix size of 1200: 1.03379e+11 flops/s
Max value: 1.16077e+11 flops/s

Performance counter stats for './foxbench -c 6 -m128MiB -i 100':

    14 303 452 889      cycles:u
    43 729 754 591      instructions:u
       662 146 053      branches:u
           821 172      branch-misses:u
     1 298 960 922      cache-references:u
       245 789 186      cache-misses:u
        22 304 605      LLC-loads:u
        11 449 496      LLC-load-misses:u
    17 251 947 769      L1-dcache-loads:u
       690 453 945      L1-dcache-load-misses:u
           362 523      dTLB-load-misses:u
           365 987      dTLB-store-misses:u
            27 082      iTLB-load-misses:u

       0,659593896 seconds time elapsed

       3,509414000 seconds user
       0,064754000 seconds sys
```

### Points of interest

- src/squarematrix.h - Implementation of a square matrix for multiplication meant for use on extremely large matrices that do not fit in the cache and rely on streaming from RAM.

### Thoughts

After trying to optimize it further, I hit a wall at 70–80% of BLAS performance. Looking at perf, it seems like a lot of performance is still hidden in memory access. After running `perf record`, it seems that most of the `LLC-load-misses` and `L1-dcache-load-misses` occur inside the `multiplyAddPacks` call. Considering that BLAS afaik has a hand-coded assembly kernel, this would explain why my C++ implementation might not fully catch up. However, I'm leaving it there to keep it reasonably easy to understand.
