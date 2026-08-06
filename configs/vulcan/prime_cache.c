/*
 * cache_prime.c
 * gcc -O0 -static -o prime_cache prime_cache.c -I../../include
 * -L../../util/m5/build/x86/out -lm5
 *
 * gem5 workload that primes (warms) a 16KB, 1-level cache.
 *
 * Strategy
 * --------
 * A 16KB cache with a typical 64-byte cache line has 256 lines.
 * We allocate a 16KB buffer aligned to the cache size, then touch
 * every cache line with a read-modify-write so every line is brought
 * in as MODIFIED (dirty).  A second pass verifies all lines are hot
 * by reading them back — if the cache is properly primed the second
 * pass should generate zero misses.
 *
 * gem5 magic instructions (m5ops) are used to:
 *   - reset stats  before the region of interest (ROI)
 *   - dump  stats  after  the ROI
 * so that cache miss/hit counters reflect only the priming phase.
 *
 * Build (bare-metal / syscall-emulation):
 *   arm-linux-gnueabihf-gcc -O1 -o prime_cache prime_cache.c
 *   x86_64-linux-gnu-gcc    -O1 -o prime_cache prime_cache.c
 *
 * Run in gem5 SE mode:
 *   ./build/X86/gem5.opt configs/example/se.py \
 *       --cpu-type=TimingSimpleCPU \
 *       --caches --l1d_size=16kB --l1d_assoc=4 \
 *       --cacheline_size=64 \
 *       -c cache_primer
 */

#include <gem5/m5ops.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Cache geometry — adjust to match your gem5 configuration           */
/* ------------------------------------------------------------------ */

#define CACHE_SIZE_BYTES (16 * 1024) /* 16 KB                     */
#define CACHE_LINE_BYTES 64          /* bytes per cache line       */
#define NUM_CACHE_LINES (CACHE_SIZE_BYTES / CACHE_LINE_BYTES) /* 256 */

/* ------------------------------------------------------------------ */
/* Workload                                                            */
/* ------------------------------------------------------------------ */

/* Prevent the compiler from optimizing away the accesses. */
volatile uint8_t sink;

int
main(void)
{
    /* Use a static buffer aligned to the cache size, instead of
     * aligned_alloc(). gem5 SE mode doesn't fully emulate some syscalls
     * glibc's malloc/tcache relies on (mprotect, rseq, set_robust_list are
     * silently ignored), which corrupts heap metadata and makes the final
     * free() abort with "double free or corruption" even though the access
     * pattern here is correct. A static buffer sidesteps malloc entirely. */
    static uint8_t buf[CACHE_SIZE_BYTES]
        __attribute__((aligned(CACHE_SIZE_BYTES)));

    /* -------------------------------------------------------------- */
    /* Phase 0: cold initialization (outside ROI).                    */
    /* Write the buffer so pages are faulted in before we measure.    */
    /* -------------------------------------------------------------- */
    for (int i = 0; i < CACHE_SIZE_BYTES; i++) {
        buf[i] = 0;
    }

    m5_reset_stats(0, 0);

    /* -------------------------------------------------------------- */
    /* Phase 1: Prime — touch every cache line once (read + write).   */
    /* Stride exactly one cache line to hit a unique line each time.  */
    /* -------------------------------------------------------------- */
    for (int line = 0; line < NUM_CACHE_LINES; line++) {
        int offset = line * CACHE_LINE_BYTES;
        buf[offset] = 1; /* read-modify-write → MODIFIED state */
    }
    m5_dump_stats(0, 0);
    m5_reset_stats(0, 0);

    /* -------------------------------------------------------------- */
    /* Phase 2: Verify — all lines should now be cache-resident.      */
    /* This pass should see 0 demand misses if the cache is primed.   */
    /* -------------------------------------------------------------- */
    uint64_t checksum = 0;
    for (int line = 0; line < NUM_CACHE_LINES; line++) {
        checksum += buf[line * CACHE_LINE_BYTES];
    }
    sink = (uint8_t)checksum; /* prevent dead-code elimination   */
    m5_dump_stats(0, 0);

    return 0;
}
