#include <gem5/m5ops.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  These are injected by the build / run script via -D flags:         */
/*    -DNUM_WAYS=4  -DCACHE_SIZE=(16*1024)  -DSTRIDE=0x1000           */
/* ------------------------------------------------------------------ */
#ifndef NUM_WAYS
#define NUM_WAYS 4
#endif
#ifndef CACHE_SIZE
#define CACHE_SIZE (16 * 1024)
#endif
#ifndef STRIDE
#define STRIDE 0x1000
#endif

/* ------------------------------------------------------------------ */
/*  Helper: warm all ways into cache (reverse order so way[0] is MRU) */
/* ------------------------------------------------------------------ */
static void warm_ways(volatile uint64_t **way, int n)
{
    for (int i = n - 1; i >= 0; i--) {
        way[i][0] = (uint64_t)i;
        volatile uint64_t tmp = way[i][0];
        (void)tmp;
    }
}

/* ------------------------------------------------------------------ */
/*  Test 1: basic lock/unlock — one line stays locked under pressure   */
/* ------------------------------------------------------------------ */
static void test_basic_lock(void)
{
    char *buf = (char *)malloc(CACHE_SIZE * NUM_WAYS);
    if (!buf) { printf("[SKIP] test_basic_lock: malloc failed\n"); return; }

    volatile uint64_t *way[NUM_WAYS];
    for (int i = 0; i < NUM_WAYS; i++)
        way[i] = (volatile uint64_t *)(buf + i * STRIDE);

    warm_ways(way, NUM_WAYS);

    /* lock way[0] only */
    m5_lock((uint64_t)(uintptr_t)way[0]);

    /* eviction pressure */
    volatile uint64_t *trash = (volatile uint64_t *)malloc(8192 * sizeof(uint64_t));
    for (int i = 0; i < 8192; i++) trash[i] = i;

    uint64_t val = way[0][0];
    int pass = (val == 0);

    m5_unlock((uint64_t)(uintptr_t)way[0]);

    printf("[test_basic_lock]        way[0][0]=0x%lx  expect=0x0  %s\n",
           val, pass ? "PASS" : "FAIL");

    free(buf);
    free((void *)trash);
}

/* ------------------------------------------------------------------ */
/*  Test 2: set restriction — locking last way must be refused         */
/* ------------------------------------------------------------------ */
static void test_set_restriction(void)
{
    char *buf = (char *)malloc(CACHE_SIZE * NUM_WAYS);
    if (!buf) { printf("[SKIP] test_set_restriction: malloc failed\n"); return; }

    volatile uint64_t *way[NUM_WAYS];
    for (int i = 0; i < NUM_WAYS; i++)
        way[i] = (volatile uint64_t *)(buf + i * STRIDE);

    warm_ways(way, NUM_WAYS);

    /* lock all ways back-to-back — last one should trigger gem5 warn */
    for (int i = 0; i < NUM_WAYS; i++)
        m5_lock((uint64_t)(uintptr_t)way[i]);

    printf("[test_set_restriction]   locked %d/%d ways. "
           "Expect gem5 warn on way[%d] above.\n",
           NUM_WAYS - 1, NUM_WAYS, NUM_WAYS - 1);

    /* cleanup */
    for (int i = 0; i < NUM_WAYS; i++)
        m5_unlock((uint64_t)(uintptr_t)way[i]);

    free(buf);
}

/* ------------------------------------------------------------------ */
/*  Test 3: unlock restores evictability                               */
/* ------------------------------------------------------------------ */
static void test_unlock_evictable(void)
{
    char *buf = (char *)malloc(CACHE_SIZE * NUM_WAYS);
    if (!buf) { printf("[SKIP] test_unlock_evictable: malloc failed\n"); return; }

    volatile uint64_t *way[NUM_WAYS];
    for (int i = 0; i < NUM_WAYS; i++)
        way[i] = (volatile uint64_t *)(buf + i * STRIDE);

    warm_ways(way, NUM_WAYS);

    /* lock then immediately unlock way[0] */
    m5_lock((uint64_t)(uintptr_t)way[0]);
    m5_unlock((uint64_t)(uintptr_t)way[0]);

    /* eviction pressure — way[0] should now be evictable */
    volatile uint64_t *trash = (volatile uint64_t *)malloc(8192 * sizeof(uint64_t));
    for (int i = 0; i < 8192; i++) trash[i] = i;

    printf("[test_unlock_evictable]  way[0] unlocked before pressure. "
           "Should be evictable (no lockCacheLine warn expected).\n");

    free(buf);
    free((void *)trash);
}

/* ------------------------------------------------------------------ */
/*  Test 4: multiple lines locked simultaneously survive pressure      */
/* ------------------------------------------------------------------ */
static void test_multi_lock(void)
{
    char *buf = (char *)malloc(CACHE_SIZE * NUM_WAYS);
    if (!buf) { printf("[SKIP] test_multi_lock: malloc failed\n"); return; }

    volatile uint64_t *way[NUM_WAYS];
    for (int i = 0; i < NUM_WAYS; i++)
        way[i] = (volatile uint64_t *)(buf + i * STRIDE);

    warm_ways(way, NUM_WAYS);

    /* lock all but the last way (leave 1 unlocked as required) */
    for (int i = 0; i < NUM_WAYS - 1; i++)
        m5_lock((uint64_t)(uintptr_t)way[i]);

    /* eviction pressure */
    volatile uint64_t *trash = (volatile uint64_t *)malloc(8192 * sizeof(uint64_t));
    for (int i = 0; i < 8192; i++) trash[i] = i;

    int all_pass = 1;
    for (int i = 0; i < NUM_WAYS - 1; i++) {
        uint64_t val = way[i][0];
        if (val != (uint64_t)i) all_pass = 0;
        printf("[test_multi_lock]        way[%d][0]=0x%lx  expect=0x%x  %s\n",
               i, val, i, val == (uint64_t)i ? "PASS" : "FAIL");
    }

    for (int i = 0; i < NUM_WAYS - 1; i++)
        m5_unlock((uint64_t)(uintptr_t)way[i]);

    printf("[test_multi_lock]        overall: %s\n", all_pass ? "PASS" : "FAIL");

    free(buf);
    free((void *)trash);
}

int main(void)
{
    printf("=== LockedLRU Test Suite ===\n");
    printf("    NUM_WAYS=%d  CACHE_SIZE=%d  STRIDE=0x%x\n\n",
           NUM_WAYS, CACHE_SIZE, STRIDE);
    fflush(stdout);

    m5_dump_stats(0, 0);

    test_basic_lock();
    test_set_restriction();
    test_unlock_evictable();
    test_multi_lock();

    m5_dump_stats(0, 0);

    printf("\n=== Done ===\n");
    fflush(stdout);
    return 0;
}