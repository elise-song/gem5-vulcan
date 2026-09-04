/*
* This file generates histogram for specific machine
*
* Copyright (C) 2020
* Author: Shuwen Deng <shuwen.deng@yale.edu>
* Date:   02.04.2020
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*
*/

// gcc timing_histogram.c -std=gnu99 -pthread -DDO_READ=1 -o timing_histogram_read
// gcc timing_histogram.c -std=gnu99 -pthread -DDO_WRITE=1 -o timing_histogram_write
// gcc timing_histogram.c -std=gnu99 -pthread -DDO_FLUSH=1 -o timing_histogram_flush


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <immintrin.h>
#include <x86intrin.h>
#include <dlfcn.h>
#include <pthread.h>
#include <time.h>

#ifndef DO_READ
#define DO_READ 0
#endif
#ifndef DO_WRITE
#define DO_WRITE 0
#endif
#ifndef DO_FLUSH
#define DO_FLUSH 0
#endif

#ifndef L1_CACHE_SIZE
#define L1_CACHE_SIZE 0x8000
#endif
#ifndef L1_ASSOC
#define L1_ASSOC 8
#endif
#define CACHE_LINE 0x40
#define L1_CACHE_SET L1_CACHE_SIZE/L1_ASSOC/CACHE_LINE

#ifndef L2_CACHE_SIZE
#define L2_CACHE_SIZE 0x40000
#endif
#ifndef L2_ASSOC
#define L2_ASSOC 8
#endif
#define L2_CACHE_SET L2_CACHE_SIZE/L2_ASSOC/CACHE_LINE

#ifndef L3_CACHE_SIZE
#define L3_CACHE_SIZE 0xA00000
#endif
#ifndef L3_ASSOC
#define L3_ASSOC 20
#endif
#define L3_CACHE_SET L3_CACHE_SIZE/L3_ASSOC/CACHE_LINE


#define L1_HIT_CLEAN 0
#define L2_HIT_CLEAN 1
#define L3_HIT_CLEAN 2
#define REMOTE_L1_HIT_CLEAN 3
#define REMOTE_L2_HIT_CLEAN 4
#define REMOTE_L3_HIT_CLEAN 5

#define L1_HIT_DIRTY 6
#define L2_HIT_DIRTY 7
#define L3_HIT_DIRTY 8
#define REMOTE_L1_HIT_DIRTY 9
#define REMOTE_L2_HIT_DIRTY 10
#define REMOTE_L3_HIT_DIRTY 11
#define DRAM_HIT 12

#define L1_REMOTE_L1_HIT_CLEAN 13
#define L1_REMOTE_L2_HIT_CLEAN 14
#define L1_REMOTE_L3_HIT_CLEAN 15
#define L2_REMOTE_L1_HIT_CLEAN 16
#define L2_REMOTE_L2_HIT_CLEAN 17
#define L2_REMOTE_L3_HIT_CLEAN 18
#define L3_REMOTE_L1_HIT_CLEAN 19
#define L3_REMOTE_L2_HIT_CLEAN 20
#define L3_REMOTE_L3_HIT_CLEAN 21

#define READ 0
#define WRITE 1
#define CLFLUSH 2


#define NUM_CALIBRE 66
#ifndef NUM_CALIBRE_SINGLE
#define NUM_CALIBRE_SINGLE 22
#endif

#define MAX_CYCLE 2500

const int l1_way_size=L1_CACHE_SET*L1_ASSOC;
const int l2_way_size=L2_CACHE_SET*L2_ASSOC;
int l1_g_stride=L1_CACHE_SET*L1_ASSOC;
char * chain;
char *chain_evict;
int ratio=((L2_CACHE_SET)/(L1_CACHE_SET));


char* probe;
char probe_arr[16*L3_CACHE_SIZE];

int histogram[NUM_CALIBRE][MAX_CYCLE]={0};
int coarse_histogram[NUM_CALIBRE][MAX_CYCLE/10]={0};

char* start[320*8+1];

int threadshold = 200;

char* conflict_set[320*8];
char* lines[8*320];

int t=0;

int conflict_bar = 0;

// --- "Remote core" worker thread -------------------------------------
//
// The REMOTE_* / L*_REMOTE_L*_HIT_CLEAN categories need one of the cache
// touches to genuinely happen on a different core than the one that later
// measures the access. gem5 SE mode has no kernel scheduler, so
// sched_setaffinity() can't migrate an already-running thread between
// cores -- it only records bookkeeping. Instead, a persistent pthread is
// spawned once at startup. Under gem5 SE mode, pthread_create() (via
// clone()) is handed the first idle (Halted) ThreadContext in the system,
// so as long as the board only starts one core active, this worker is
// pinned to a genuinely different core for its entire lifetime -- no
// migration required. See the matching timing_histogram.py, which leaves
// every core but the first Halted for exactly this reason.

enum {
  WORKER_IDLE = 0,
  WORKER_TOUCH_CLEAN,
  WORKER_TOUCH_DIRTY,
  WORKER_EVICT_L1,
  WORKER_EVICT_L2_FULL,
  WORKER_EVICT_CONFLICT,
  WORKER_EXIT,
};

typedef struct {
  volatile int cmd;
  volatile unsigned req_seq;
  volatile unsigned ack_seq;
} worker_channel_t;

static worker_channel_t worker_channel = {WORKER_IDLE, 0, 0};
static pthread_t worker_tid;

void shuffle(char **array, int n, int length){
  long i, j;
  char* tmp;
  srand(time(NULL));
  for(i = 0; i < n; i++){
    j = rand() % length;
    tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
  }
}

int probe_array(char **set, int size, char *candidate){
  //read candidate
  asm __volatile__ (
       "lfence              \n"
       "movq (%%rcx),  %%rax     \n"
       "movq 64(%%rcx), %%rax     \n"
       "movq 448(%%rcx),%%rax     \n"
       "movq 256(%%rcx),%%rax     \n"
       "movq 384(%%rcx),%%rax     \n"
       "movq 320(%%rcx),%%rax     \n"
       "movq 192(%%rcx),%%rax     \n"
       "movq 128(%%rcx),%%rax     \n"
       "lfence              \n"
       :
       : "c" (candidate)
       : "%esi", "%edx");

  // read l
  for (int i = 0; i < size; ++i)
  {
    asm __volatile__ (
       "lfence              \n"
       "movq (%%rcx),  %%rax     \n"
       "movq 64(%%rcx), %%rax     \n"
       "movq 448(%%rcx),%%rax     \n"
       "movq 256(%%rcx),%%rax     \n"
       "movq 384(%%rcx),%%rax     \n"
       "movq 320(%%rcx),%%rax     \n"
       "movq 192(%%rcx),%%rax     \n"
       "movq 128(%%rcx),%%rax     \n"
       "lfence              \n"
       :
       : "c" (set[i])
       : "%esi", "%edx");
  }

  // measure time to read candidate

  asm __volatile__ (
       "lfence              \n"
       "rdtsc               \n"
       "movl %%eax, %%esi   \n"
       "movq (%%rcx),  %%rax     \n"
       "lfence              \n"
       "movq 64(%%rcx), %%rax     \n"
       "lfence              \n"
       "movq 448(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 256(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 384(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 320(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 192(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 128(%%rcx),%%rax     \n"
       "lfence              \n"
       "rdtsc               \n"
       "subl %%esi, %%eax   \n"
       : "=a" (t)
       : "c" (candidate)
       : "%esi", "%edx");

  return (t > threadshold );
}

// --- Shared cache-touch primitives ------------------------------------
// Used both locally (by the measuring thread) and remotely (dispatched to
// the worker thread), so the "local" and "remote" variants of each
// category run identical code -- only which core executes it differs.

static inline void touch_clean(char *p) {
  asm __volatile__ (
       "lfence              \n"
       "movq (%%rcx),  %%rax     \n"
       "movq 64(%%rcx), %%rax     \n"
       "movq 448(%%rcx),%%rax     \n"
       "movq 256(%%rcx),%%rax     \n"
       "movq 384(%%rcx),%%rax     \n"
       "movq 320(%%rcx),%%rax     \n"
       "movq 192(%%rcx),%%rax     \n"
       "movq 128(%%rcx),%%rax     \n"
       "lfence              \n"
       :
       : "c" (p)
       : "%esi", "%edx");
}

static inline void touch_dirty(char *p) {
  asm __volatile__ (
           "sfence              \n"
           "movq %%rcx, (%%rcx)       \n"
           "sfence              \n"
           "movq %%rcx, 64(%%rcx)     \n"
           "sfence              \n"
           "movq %%rcx, 448(%%rcx)     \n"
           "sfence              \n"
           "movq %%rcx, 256(%%rcx)     \n"
           "sfence              \n"
           "movq %%rcx, 384(%%rcx)     \n"
           "sfence              \n"
           "movq %%rcx, 320(%%rcx)     \n"
           "sfence              \n"
           "movq %%rcx, 192(%%rcx)     \n"
           "sfence              \n"
           "movq %%rcx, 128(%%rcx)     \n"
           "sfence              \n"
           :
           : "c" (p)
           : "%esi", "%edx");
}

// Evict a line from L1 (touch the other L1_ASSOC-1 ways of its set).
static inline void evict_l1(char **start_arr) {
  for (int i = 1; i < 9; ++i) touch_clean(start_arr[i]);
}

// Evict a line from L2 (touch enough conflicting lines to fill the set
// across every core sharing it, ratio*L2_ASSOC ways' worth).
static inline void evict_l2_full(char **start_arr) {
  for (int i = 1; i < 65; ++i) touch_clean(start_arr[i]);
}

// Evict a line from L2 into L3 (touch the precomputed L3 eviction set).
static inline void evict_conflict(void) {
  for (int i = 0; i < conflict_bar; ++i) touch_clean(conflict_set[i]);
}

// --- Worker thread request/response -----------------------------------

static void worker_request(int cmd) {
  worker_channel.cmd = cmd;
  __atomic_thread_fence(__ATOMIC_RELEASE);
  unsigned seq = ++worker_channel.req_seq;
  while (worker_channel.ack_seq != seq) {
    __asm__ __volatile__("pause" ::: "memory");
  }
}

static void *worker_thread_fn(void *arg) {
  unsigned last_seq = 0;
  for (;;) {
    while (worker_channel.req_seq == last_seq) {
      __asm__ __volatile__("pause" ::: "memory");
    }
    last_seq = worker_channel.req_seq;
    int cmd = worker_channel.cmd;
    int should_exit = (cmd == WORKER_EXIT);
    switch (cmd) {
      case WORKER_TOUCH_CLEAN:    touch_clean(start[0]);   break;
      case WORKER_TOUCH_DIRTY:    touch_dirty(start[0]);   break;
      case WORKER_EVICT_L1:       evict_l1(start);         break;
      case WORKER_EVICT_L2_FULL:  evict_l2_full(start);    break;
      case WORKER_EVICT_CONFLICT: evict_conflict();        break;
      default: break;
    }
    __atomic_thread_fence(__ATOMIC_RELEASE);
    worker_channel.ack_seq = last_seq;
    if (should_exit) return NULL;
  }
}



unsigned long test_delay(char** start, char* chain_tar, int sec, int mea_type) {

  unsigned long t=0;

  if(sec==L1_REMOTE_L1_HIT_CLEAN||sec==L1_REMOTE_L2_HIT_CLEAN||sec==L1_REMOTE_L3_HIT_CLEAN||sec==L2_REMOTE_L1_HIT_CLEAN||sec==L2_REMOTE_L2_HIT_CLEAN||sec==L2_REMOTE_L3_HIT_CLEAN||sec==L3_REMOTE_L1_HIT_CLEAN||sec==L3_REMOTE_L2_HIT_CLEAN||sec==L3_REMOTE_L3_HIT_CLEAN){
    touch_clean(start[0]);
  }

  if(sec==L2_REMOTE_L1_HIT_CLEAN||sec==L2_REMOTE_L2_HIT_CLEAN||sec==L2_REMOTE_L3_HIT_CLEAN){
    evict_l1(start);
  }


  if(sec==L3_REMOTE_L1_HIT_CLEAN||sec==L3_REMOTE_L2_HIT_CLEAN||sec==L3_REMOTE_L3_HIT_CLEAN){
    evict_conflict();
  }

  // From here on, "remote" work is dispatched to the worker thread
  // (pinned to a different core for its whole lifetime) instead of
  // migrating this thread.
  if(sec==L1_REMOTE_L1_HIT_CLEAN||sec==L1_REMOTE_L2_HIT_CLEAN||sec==L1_REMOTE_L3_HIT_CLEAN||sec==L2_REMOTE_L1_HIT_CLEAN||sec==L2_REMOTE_L2_HIT_CLEAN||sec==L2_REMOTE_L3_HIT_CLEAN||sec==L3_REMOTE_L1_HIT_CLEAN||sec==L3_REMOTE_L2_HIT_CLEAN||sec==L3_REMOTE_L3_HIT_CLEAN){
    worker_request(WORKER_TOUCH_CLEAN);
  }

  if(sec==L1_REMOTE_L2_HIT_CLEAN||sec==L2_REMOTE_L2_HIT_CLEAN||sec==L3_REMOTE_L2_HIT_CLEAN){
    worker_request(WORKER_EVICT_L1);
  }

  if(sec==L1_REMOTE_L3_HIT_CLEAN||sec==L2_REMOTE_L3_HIT_CLEAN||sec==L3_REMOTE_L3_HIT_CLEAN){
    worker_request(WORKER_EVICT_CONFLICT);
  }


  if(sec==L1_HIT_CLEAN||sec==L2_HIT_CLEAN||sec==L3_HIT_CLEAN||sec==DRAM_HIT){
      touch_clean(start[0]);
  }else if(sec==REMOTE_L1_HIT_CLEAN||sec==REMOTE_L2_HIT_CLEAN||sec==REMOTE_L3_HIT_CLEAN){
      worker_request(WORKER_TOUCH_CLEAN);
  }else if(sec==L1_HIT_DIRTY||sec==L2_HIT_DIRTY||sec==L3_HIT_DIRTY){
      touch_dirty(start[0]);
  }else if(sec==REMOTE_L1_HIT_DIRTY||sec==REMOTE_L2_HIT_DIRTY||sec==REMOTE_L3_HIT_DIRTY){
      worker_request(WORKER_TOUCH_DIRTY);
  }


    // possible eviction to certain level of cache
  if(sec==L2_HIT_CLEAN||sec==L2_HIT_DIRTY){
      evict_l2_full(start);
  } else if(sec==REMOTE_L2_HIT_CLEAN||sec==REMOTE_L2_HIT_DIRTY){
      worker_request(WORKER_EVICT_L2_FULL);
  } else if(sec==L3_HIT_CLEAN||sec==L3_HIT_DIRTY){
      evict_conflict();
  } else if(sec==REMOTE_L3_HIT_CLEAN||sec==REMOTE_L3_HIT_DIRTY){
      worker_request(WORKER_EVICT_CONFLICT);
  } else if (sec==DRAM_HIT){
        asm __volatile__ (
           "mfence              \n"
           "clflush (%%rcx)     \n"
           "clflush 64(%%rcx)     \n"
           "clflush 448(%%rcx)     \n"
           "clflush 256(%%rcx)     \n"
           "clflush 384(%%rcx)     \n"
           "clflush 320(%%rcx)     \n"
           "clflush 192(%%rcx)     \n"
           "clflush 128(%%rcx)     \n"
           "mfence              \n"
           :
           : "c" (start[0])
           : "%esi", "%edx");
  }


  //measure latency
  if (mea_type==READ){

     asm __volatile__ (
       "lfence              \n"
       "rdtsc               \n"
       "movl %%eax, %%esi   \n"
       "movq (%%rcx),  %%rax     \n"
       "lfence              \n"
       "movq 64(%%rcx), %%rax     \n"
       "lfence              \n"
       "movq 448(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 256(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 384(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 320(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 192(%%rcx),%%rax     \n"
       "lfence              \n"
       "movq 128(%%rcx),%%rax     \n"
       "lfence              \n"
       "rdtsc               \n"
       "subl %%esi, %%eax   \n"
       : "=a" (t)
       : "c" (start[0])
       : "%esi", "%edx");


  } else if (mea_type==WRITE){


     asm __volatile__ (
       "mfence              \n"
       "rdtsc               \n"
       "movl %%eax, %%esi   \n"
       "movq %%rcx, (%%rcx)       \n"
       "mfence              \n"
       "movq %%rcx, 64(%%rcx)     \n"
       "mfence              \n"
       "movq %%rcx, 448(%%rcx)     \n"
       "mfence              \n"
       "movq %%rcx, 256(%%rcx)     \n"
       "mfence              \n"
       "movq %%rcx, 384(%%rcx)     \n"
       "mfence              \n"
       "movq %%rcx, 320(%%rcx)     \n"
       "mfence              \n"
       "movq %%rcx, 192(%%rcx)     \n"
       "mfence              \n"
       "movq %%rcx, 128(%%rcx)     \n"
       "mfence              \n"
       "rdtsc               \n"
       "subl %%esi, %%eax   \n"
       : "=a" (t)
       : "c" (start[0])
       : "%esi", "%edx");


  } else if (mea_type==CLFLUSH){


      asm __volatile__ (
       "lfence              \n"
       "rdtsc               \n"
       "movl %%eax, %%esi   \n"
       "mfence              \n"
       "clflush (%%rcx)     \n"
       "mfence              \n"
       "clflush 64(%%rcx)     \n"
       "mfence              \n"
       "clflush 448(%%rcx)     \n"
       "mfence              \n"
       "clflush 256(%%rcx)     \n"
       "mfence              \n"
       "clflush 384(%%rcx)     \n"
       "mfence              \n"
       "clflush 320(%%rcx)     \n"
       "mfence              \n"
       "clflush 192(%%rcx)     \n"
       "mfence              \n"
       "clflush 128(%%rcx)     \n"
       "mfence              \n"
       "rdtsc               \n"
       "subl %%esi, %%eax   \n"
       : "=a" (t)
       : "c" (start[0])
       : "%esi", "%edx");
  }

  return t;

}



int main(int argc, char *argv[]) {

  char file_long[100];
  sprintf(file_long, "histogram_%s.out", argv[1]);

  char file_name[100];
  sprintf(file_name, "coarse_histogram_%s.out", argv[1]);


  FILE *fp = fopen(file_long, "w");
  FILE *fp_coarse = fopen(file_name, "w");


  long long ave_time_cycle_arr[NUM_CALIBRE] =  {0};

  volatile pid_t *maintain_arr;

  int test_cnt=10000;

  //probe_arr
  for (int i = 0; i < 16*L3_CACHE_SIZE; ++i)
  {
    probe_arr[i] = i;
  }

  probe = probe_arr;

  for(int i=0;i< sizeof(start)/sizeof(start[0]) ;i++){// 8 way cache
    start[i]=&probe[ i*l1_way_size];
  }

  unsigned int t;



  // create eviction set for LLC

  for (int i = 0; i < 320*8; ++i)
  {
      lines[i]=start[i+1];
  }

  // randomize
  shuffle(lines, 320*8, 320*8);



  for (int candidate = 0; candidate < 320*8; ++candidate)
  {
    if(!(probe_array(conflict_set, conflict_bar, lines[candidate]))){
      conflict_set[conflict_bar++] = lines[candidate];
    }
  }
  //printf("conflict_bar=%d\n", conflict_bar);
  for (int i = 0; i < conflict_bar; ++i)
  {
    //printf("conflict_set[%d]=%p\n", i, conflict_set[i]);
  }

  // Spawn the persistent "remote core" worker. Under gem5 SE mode this
  // pthread is handed the first idle ThreadContext in the system (see
  // timing_histogram.py), so it stays pinned to a genuinely different
  // core than this (the measuring) thread for the rest of the run.
  if (pthread_create(&worker_tid, NULL, worker_thread_fn, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  int counter_hist=0;
  int ord_calibre;

  //calibration
  // READ
#if DO_READ
  for (int ord_calibre = 0; ord_calibre < NUM_CALIBRE; ++ord_calibre)
  {
    counter_hist = ord_calibre;
    printf("Generating histogram for timing type %d ...\n", counter_hist+1);
    //add dummy computation to make sure the data load into L1/L2 from memory
    for(int j=0;j <100;j++){
        t=test_delay(start,chain,ord_calibre, READ);
        for (int i = 0; i < 1000000; ++i){
              t+=i;
        }
    }
    for(int i=0;i<test_cnt;i++){
      t=test_delay(start,chain,ord_calibre, READ);
      ave_time_cycle_arr[counter_hist] += t;
      if(t<MAX_CYCLE){
        histogram[counter_hist][t]++;
        coarse_histogram[counter_hist][t/10]++;
      }
      else {
        histogram[counter_hist][MAX_CYCLE-1]++;
        coarse_histogram[counter_hist][MAX_CYCLE/10-1]++;
      }
    }
  }
#endif

  // WRITE
#if DO_WRITE
  for (int ord_calibre = 0; ord_calibre < NUM_CALIBRE; ++ord_calibre)
  {
    counter_hist = ord_calibre;
    printf("Generating histogram for timing type %d ...\n", counter_hist+1);
    //add dummy computation to make sure the data load into L1/L2 from memory
    for(int j=0;j <100;j++){
        t=test_delay(start,chain,ord_calibre, WRITE);
        for (int i = 0; i < 1000000; ++i){
              t+=i;
        }
    }
    for(int i=0;i<test_cnt;i++){
      t=test_delay(start,chain,ord_calibre, WRITE);
      ave_time_cycle_arr[counter_hist] += t;
      if(t<MAX_CYCLE){
        histogram[counter_hist][t]++;
        coarse_histogram[counter_hist][t/10]++;
      }
      else {
        histogram[counter_hist][MAX_CYCLE-1]++;
        coarse_histogram[counter_hist][MAX_CYCLE/10-1]++;
      }
    }
  }
#endif

  // FLUSH
#if DO_FLUSH
  for (int ord_calibre = 0; ord_calibre < NUM_CALIBRE; ++ord_calibre)
  {
    counter_hist = ord_calibre;
    printf("Generating histogram for timing type %d ...\n", counter_hist+1);
    //add dummy computation to make sure the data load into L1/L2 from memory
    for(int j=0;j <100;j++){
        t=test_delay(start,chain,ord_calibre, CLFLUSH);
        for (int i = 0; i < 1000000; ++i){
              t+=i;
        }
    }
    for(int i=0;i<test_cnt;i++){
      t=test_delay(start,chain,ord_calibre, CLFLUSH);
      ave_time_cycle_arr[counter_hist] += t;
      if(t<MAX_CYCLE){
        histogram[counter_hist][t]++;
        coarse_histogram[counter_hist][t/10]++;
      }
      else {
        histogram[counter_hist][MAX_CYCLE-1]++;
        coarse_histogram[counter_hist][MAX_CYCLE/10-1]++;
      }
    }
  }
#endif

  worker_request(WORKER_EXIT);
  pthread_join(worker_tid, NULL);

  int min_time_diff = MAX_CYCLE;
  int min_time_cycle_arr[NUM_CALIBRE] =  {0};
  int min_time_fre_arr[NUM_CALIBRE] = {0};


  for(int i=0;i<MAX_CYCLE;i++){
  	for (int j = 0; j < NUM_CALIBRE; ++j)
  	{
  		fprintf(fp, "%d\t", histogram[j][i]);
  	}
  	fprintf(fp, "\n");



    for (int j = 0; j < NUM_CALIBRE; ++j)
    {
      if (histogram[j][i]>min_time_fre_arr[j]){
        min_time_fre_arr[j] = histogram[j][i];
        min_time_cycle_arr[j] = i;
      }
    }
  }
  for(int i=0;i<MAX_CYCLE/10;i++){
  	for (int j = 0; j < NUM_CALIBRE; ++j)
  	{
  		fprintf(fp_coarse, "%d\t", coarse_histogram[j][i]);
  	}
  	fprintf(fp_coarse, "\n");
  }
  for (int j = 0; j < NUM_CALIBRE; ++j)
  {
    ave_time_cycle_arr[j] = ave_time_cycle_arr[j]/test_cnt;
  }
  int a;
  int tmp_min_cycle_arr[NUM_CALIBRE] = {};
  printf("The maximum cycle number of 66 types of timings are given below \n");
  for (int j = 0; j < NUM_CALIBRE; ++j)
  	{
  		printf("%d,", min_time_cycle_arr[j]);
  	}
  	printf("\n");

  printf("The average cycle number of 66 types of timings are given below \n");
  for (int j = 0; j < NUM_CALIBRE; ++j)
  	{
  		printf("%lld,", ave_time_cycle_arr[j]);
  	}
  	printf("\n");


  for (int i = 0; i < NUM_CALIBRE; ++i)
  {
    tmp_min_cycle_arr[i]=min_time_cycle_arr[i];
  }

  for (int i = 0; i < NUM_CALIBRE; ++i)
  {
    for (int j = 0; j < NUM_CALIBRE; ++j)
    {
      if (tmp_min_cycle_arr[i] < tmp_min_cycle_arr[j])
        {
        a =  tmp_min_cycle_arr[i];
        tmp_min_cycle_arr[i] = tmp_min_cycle_arr[j];
        tmp_min_cycle_arr[j] = a;
        }
    }
  }

  printf("Histogram numbers are output to histogram_output/histogram_%s.out and histogram_output/coarse_histogram_%s.out\n", argv[1], argv[1]);

  fclose(fp);
  fclose(fp_coarse);

  return 0;
}
