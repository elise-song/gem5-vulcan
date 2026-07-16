/* compile:
gcc -O0 -static configs/vulcan/calibrate.c -o calibrate
*/

/* Measures average cache hit vs. flush+miss latency (in rdtscp cycles) so
 * CACHE_HIT_THRESHOLD in spectre.c can be tuned for a given gem5 CPU/cache
 * config. Run under the same gem5 config you'll run spectre under. */
#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

uint8_t array2[256 * 512];

int
main()
{
    volatile uint8_t *addr = &array2[123 * 512];
    unsigned int junk;
    uint64_t t1, t2;
    long hit_sum = 0, miss_sum = 0;
    int n = 200;

    /* warm up */
    *addr;

    for (int i = 0; i < n; i++) {
        /* MISS: flush then time */
        _mm_clflush(addr);
        t1 = __rdtscp(&junk);
        junk = *addr;
        t2 = __rdtscp(&junk);
        miss_sum += (long)(t2 - t1);

        /* HIT: access again immediately (should be cached now) */
        t1 = __rdtscp(&junk);
        junk = *addr;
        t2 = __rdtscp(&junk);
        hit_sum += (long)(t2 - t1);
    }

    printf("avg HIT  cycles: %ld\n", hit_sum / n);
    printf("avg MISS cycles: %ld\n", miss_sum / n);
    return 0;
}
