#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <emmintrin.h>
#include <x86intrin.h>
#include <stdint.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>

#include <math.h>

#include <time.h>

#define L1_CACHE_SIZE 32768
#define L1_ASSOC 8
#define L1_CACHE_LINE 64
#define L1_CACHE_SET  L1_CACHE_SIZE/L1_ASSOC/L1_CACHE_LINE
#define VIC_MACHINE 9
#define PROBE_SIZE 3
#define MAX_ARRAY_SIZE 262144
#define NUM_TEST 10

#define NUM_TEST_HALF 5

#define EACH_RUN 30

#define MAX_CYCLE 8000

#define NOONE_RUN 0
#define STEP0_RUN 1
#define STEP1_RUN 2
#define STEP2_RUN 3
#define STEP3_RUN 4
const int line_size=64;
const int way_size=64*64/8;
int histogram[3][8000]={0};
int status;
int u_last_step = 0;
uint64_t sum[PROBE_SIZE] = {0};
uint64_t counter[PROBE_SIZE] = {0};
uint64_t result[2] = {0};
double arr1[30*5]={0};
double arr2[30*5]={0};
double arr3[30*5]={0};
double arr4[30*5]={0};
double arr5[30*5]={0};
double arr6[30*5]={0};
char *chain_arr;
volatile pid_t *maintain_arr;

double Pvalue (const double *restrict ARRAY1, const size_t ARRAY1_SIZE, const double *restrict ARRAY2, const size_t ARRAY2_SIZE) {//calculate a p-value based on an array
  if (ARRAY1_SIZE <= 1) {
    return 1.0;
  } else if (ARRAY2_SIZE <= 1) {
    return 1.0;
  }
  double fmean1 = 0.0, fmean2 = 0.0;
  for (size_t x = 0; x < ARRAY1_SIZE; x++) {//get sum of values in ARRAY1
    if (isfinite(ARRAY1[x]) == 0) {//check to make sure this is a real numbere
      puts("Got a non-finite number in 1st array, can't calculate P-value.");
      exit(EXIT_FAILURE);
    }
    fmean1 += ARRAY1[x];
  }
  fmean1 /= ARRAY1_SIZE;
  for (size_t x = 0; x < ARRAY2_SIZE; x++) {//get sum of values in ARRAY2
    if (isfinite(ARRAY2[x]) == 0) {//check to make sure this is a real number
      puts("Got a non-finite number in 2nd array, can't calculate P-value.");
      exit(EXIT_FAILURE);
    }
    fmean2 += ARRAY2[x];
  }
  fmean2 /= ARRAY2_SIZE;
//  printf("mean1 = %lf mean2 = %lf\n", fmean1, fmean2);
  if (fmean1 == fmean2) {
    printf("the means are equal\n");
    return 1.0;//if the means are equal, the p-value is 1, leave the function
  }
  double unbiased_sample_variance1 = 0.0, unbiased_sample_variance2 = 0.0;
  for (size_t x = 0; x < ARRAY1_SIZE; x++) {//1st part of added unbiased_sample_variance
    unbiased_sample_variance1 += (ARRAY1[x]-fmean1)*(ARRAY1[x]-fmean1);
  }
  for (size_t x = 0; x < ARRAY2_SIZE; x++) {
    unbiased_sample_variance2 += (ARRAY2[x]-fmean2)*(ARRAY2[x]-fmean2);
  }
//  printf("unbiased_sample_variance1 = %lf\tunbiased_sample_variance2 = %lf\n",unbiased_sample_variance1,unbiased_sample_variance2);//DEBUGGING
  unbiased_sample_variance1 = unbiased_sample_variance1/(ARRAY1_SIZE-1);
  unbiased_sample_variance2 = unbiased_sample_variance2/(ARRAY2_SIZE-1);
  const double WELCH_T_STATISTIC = (fmean1-fmean2)/sqrt(unbiased_sample_variance1/ARRAY1_SIZE+unbiased_sample_variance2/ARRAY2_SIZE);
  const double DEGREES_OF_FREEDOM = pow((unbiased_sample_variance1/ARRAY1_SIZE+unbiased_sample_variance2/ARRAY2_SIZE),2.0)//numerator
   /
  (
    (unbiased_sample_variance1*unbiased_sample_variance1)/(ARRAY1_SIZE*ARRAY1_SIZE*(ARRAY1_SIZE-1))+
    (unbiased_sample_variance2*unbiased_sample_variance2)/(ARRAY2_SIZE*ARRAY2_SIZE*(ARRAY2_SIZE-1))
  );
//  printf("Welch = %lf DOF = %lf\n", WELCH_T_STATISTIC, DEGREES_OF_FREEDOM);
    const double a = DEGREES_OF_FREEDOM/2;
  double value = DEGREES_OF_FREEDOM/(WELCH_T_STATISTIC*WELCH_T_STATISTIC+DEGREES_OF_FREEDOM);
  if ((isinf(value) != 0) || (isnan(value) != 0)) {
    printf("free degree1\n");
    return 1.0;
  }
  if ((isinf(value) != 0) || (isnan(value) != 0)) {
    printf("free degree2\n");
    return 1.0;
  }
 
  const double beta = lgammal(a)+0.57236494292470009-lgammal(a+0.5);
  const double acu = 0.1E-14;
  double ai;
  double cx;
  int indx;
  int ns;
  double pp;
  double psq;
  double qq;
  double rx;
  double temp;
  double term;
  double xx;
//  ifault = 0;
//Check the input arguments.
  if ( (a <= 0.0)) {// || (0.5 <= 0.0 )){
//    *ifault = 1;
//    return value;
  }
  if ( value < 0.0 || 1.0 < value )
  {
//    *ifault = 2;
    return value;
  }
/*
  Special cases.
*/
  if ( value == 0.0 || value == 1.0 )   {
    return value;
  }
  psq = a + 0.5;
  cx = 1.0 - value;
 
  if ( a < psq * value )
  {
    xx = cx;
    cx = value;
    pp = 0.5;
    qq = a;
    indx = 1;
  }
  else
  {
    xx = value;
    pp = a;
    qq = 0.5;
    indx = 0;
  }
 
  term = 1.0;
  ai = 1.0;
  value = 1.0;
  ns = ( int ) ( qq + cx * psq );
/*
  Use the Soper reduction formula.
*/
  rx = xx / cx;
  temp = qq - ai;
  if ( ns == 0 )
  {
    rx = xx;
  }
 
  for ( ; ; )
  {
    term = term * temp * rx / ( pp + ai );
    value = value + term;;
    temp = fabs ( term );
 
    if ( temp <= acu && temp <= acu * value )
    {
      value = value * exp ( pp * log ( xx ) 
      + ( qq - 1.0 ) * log ( cx ) - beta ) / pp;
 
      if ( indx )
      {
        value = 1.0 - value;
      }
      break;
    }
    ai = ai + 1.0;
    ns = ns - 1;
 
    if ( 0 <= ns )
    {
      temp = qq - ai;
      if ( ns == 0 )
      {
        rx = xx;
      }
    }
    else
    {
      temp = psq;
      psq = psq + 1.0;
    }
  }
  return value;
}
int main(int argc, char **argv) {
  srand(time(NULL));
  /* get the number of cpu's */
  cpu_set_t mycpuset;
  int numcpu = sysconf( _SC_NPROCESSORS_ONLN );
  int mycpu, cpu;
  // get our CPU 
  CPU_SET(4, &mycpuset);
  if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &mycpuset) == -1) {
        perror("sched_setaffinity");
    }
  //print_affinity();
  //printf("sched_getcpu_in_main = %d\n", sched_getcpu());
     
  size_t t=0;
  size_t t0=0;
  size_t t1=0;
  size_t t2=0;
  size_t t3=0;
  size_t t4=0;
  size_t t5=0;
  size_t t6=0;
  size_t t7=0;
  size_t t8=0;
  size_t t9=0;
  size_t t10=0;
  size_t t11=0;
  size_t t12=0;
  size_t t13=0;
  size_t t14=0;
  size_t t15=0;
  size_t t16=0;
  size_t t17=0;
  size_t t18=0;
  size_t t19=0;
  size_t t20=0;
  size_t t21=0;
  size_t t22=0;
  size_t t23=0;
  size_t t24=0;
  size_t t25=0;
  size_t t26=0;
  size_t t27=0;
  size_t t28=0;
  size_t t29=0;
  size_t t30=0;
  size_t t31=0;
  size_t t32=0;
  size_t t_r=0;
  size_t t_r0=0;
  size_t t_r1=0;
  size_t t_r2=0;
  size_t t_r3=0;
  size_t t_r4=0;
  size_t t_r5=0;
  size_t t_r6=0;
  size_t t_r7=0;
  size_t t_r8=0;
  for (int i = 0; i < 30*5; ++i)
  {
  	arr1[i] = 0;
  	arr2[i] = 0;
  	arr3[i] = 0;
  	arr4[i] = 0;
  	arr5[i] = 0;
  	arr6[i] = 0;
  }
  FILE *fp = fopen(strcat(argv[1],".output"), "a");
  FILE *fp_res = fopen(strcat(argv[1],".res"), "a");

  int c;
  pid_t pid_n, pid_l_1, pid_l_2, pid_r_1, pid_r_2;
  uint64_t a, b, d, e;

  // Map space for shared array
  chain_arr = mmap(0, 64*8*64*sizeof(char), PROT_READ|PROT_WRITE,
  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (!chain_arr) {
  perror("mmap failed for chain_arr");
  exit(1);
  }
  memset((void *)chain_arr, 0, 64*8*64*sizeof(char));

	int rand_chosen=1000;
//  initiate maintain_arr
  maintain_arr = mmap(0, 10000*sizeof(pid_t), PROT_READ|PROT_WRITE,
              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (!maintain_arr) {
    perror("mmap failed for maintain_arr");
    exit(1);
  }
  for(int i=0; i < 10000; i++){
    maintain_arr[i]=10000-i;
  }
  for(int i =0 ; i< 8*64*64; i++) /*probe_arr*/ chain_arr[i]=/*(char)*/ MAX_ARRAY_SIZE-i; //copy on write
  char* start[8*64*64];
  
  int tar_block;
  int untar_block;
  for(int i=0;i< 64*8/4;i++){
  	start[i]=&chain_arr[ i*way_size	];
  }
  // load into L1 L2
	  for(int i=0;i<8*8;i++){  
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[i])
      : );
	  }  
	  for(int i=0;i<8*8;i++){  
      asm __volatile__ (
      "mfence              \n" 
      "movq 576(%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 768(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 512(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 896(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 704(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 832(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 960(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 640(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[i]+untar_block)
      : );
	  }  

  int tmp;
  for (int i = 0; i < 1000000; ++i){
      tmp+=i;
    } 
 maintain_arr[5000] = NOONE_RUN; 
 // local attacker
 if ((pid_l_1 = fork()) < 0) {
     printf("Failed to fork process 1\n");
     exit(1);
 }
 else if (pid_l_1 == 0) {
          CPU_ZERO(&mycpuset); 
          CPU_SET(0, &mycpuset); 
          //set processor affinity 
  	if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &mycpuset) == -1) {
        perror("sched_setaffinity");
    }
 
 for (int m = 0; m < EACH_RUN; m++) 
 { 
  for (int e = 0; e < PROBE_SIZE; ++e)
   {
     sum[e] = 0;
     counter[e] = 0;
   }
    //probe = probe + 0x10*j; 
    for (int i = 0; i < NUM_TEST_HALF; ++i) 
    { 
    for (int round_rand = 0; round_rand < 2; ++round_rand) 
    	{ 
  	for (int j = 0; j < PROBE_SIZE; ++j) 
  		{ 
           
  if(maintain_arr[5000]==NOONE_RUN){
  maintain_arr[5000]=STEP0_RUN;
  }
  while(maintain_arr[5000]!=STEP0_RUN)sched_yield();
  		tar_block = rand()%4;
  		untar_block = rand()%4;
        //if (j==1 && i==299 && m==30) printf("sched_getcpu_in_step_1 = %d\n", sched_getcpu()); 
 
		maintain_arr[5000]=STEP1_RUN;
  while(maintain_arr[5000]!=STEP1_RUN)sched_yield();
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[16]+tar_block)
      : );
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[17]+tar_block)
      : );
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[18]+tar_block)
      : );
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[19]+tar_block)
      : );
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[20]+tar_block)
      : );
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[21]+tar_block)
      : );
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[22]+tar_block)
      : );
      asm __volatile__ (
      "mfence              \n" 
      "movq (%%rcx),  %%rax     \n"
      "mfence              \n" 
      "movq 64(%%rcx), %%rax     \n"
      "mfence              \n" 
      "movq 448(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 256(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 384(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 320(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 192(%%rcx),%%rax     \n"
      "mfence              \n" 
      "movq 128(%%rcx),%%rax     \n"
      "mfence              \n"
      : 
      : "c" (start[23]+tar_block)
      : );
        //if (j==1 && i==299 && m==30) printf("sched_getcpu_in_step_1 = %d\n", sched_getcpu()); 
 
		maintain_arr[5000]=STEP2_RUN;
			while(maintain_arr[5000]!=STEP3_RUN)sched_yield();
         printf("maintain_arr[6000]=%d, maintain_arr[6001]=%d\n", maintain_arr[6000], maintain_arr[6001]);	
      asm __volatile__ (
      "mfence              \n" 
      "rdtsc               \n" 
      "movl %%eax, %%esi   \n" 
      "mfence              \n" 
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
      "subl %%esi, %%eax         \n" 
      :"=a" (t0) 
      : "c" (start[16]+tar_block)
      : "%esi", "%edx");
      asm __volatile__ (
      "mfence              \n" 
      "rdtsc               \n" 
      "movl %%eax, %%esi   \n" 
      "mfence              \n" 
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
      "subl %%esi, %%eax         \n" 
      :"=a" (t1) 
      : "c" (start[17]+tar_block)
      : "%esi", "%edx");
      asm __volatile__ (
      "mfence              \n" 
      "rdtsc               \n" 
      "movl %%eax, %%esi   \n" 
      "mfence              \n" 
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
      "subl %%esi, %%eax         \n" 
      :"=a" (t2) 
      : "c" (start[18]+tar_block)
      : "%esi", "%edx");
      asm __volatile__ (
      "mfence              \n" 
      "rdtsc               \n" 
      "movl %%eax, %%esi   \n" 
      "mfence              \n" 
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
      "subl %%esi, %%eax         \n" 
      :"=a" (t3) 
      : "c" (start[19]+tar_block)
      : "%esi", "%edx");
      asm __volatile__ (
      "mfence              \n" 
      "rdtsc               \n" 
      "movl %%eax, %%esi   \n" 
      "mfence              \n" 
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
      "subl %%esi, %%eax         \n" 
      :"=a" (t4) 
      : "c" (start[20]+tar_block)
      : "%esi", "%edx");
      asm __volatile__ (
      "mfence              \n" 
      "rdtsc               \n" 
      "movl %%eax, %%esi   \n" 
      "mfence              \n" 
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
      "subl %%esi, %%eax         \n" 
      :"=a" (t5) 
      : "c" (start[21]+tar_block)
      : "%esi", "%edx");
      asm __volatile__ (
      "mfence              \n" 
      "rdtsc               \n" 
      "movl %%eax, %%esi   \n" 
      "mfence              \n" 
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
      "subl %%esi, %%eax         \n" 
      :"=a" (t6) 
      : "c" (start[22]+tar_block)
      : "%esi", "%edx");
      asm __volatile__ (
      "mfence              \n" 
      "rdtsc               \n" 
      "movl %%eax, %%esi   \n" 
      "mfence              \n" 
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
      "subl %%esi, %%eax         \n" 
      :"=a" (t7) 
      : "c" (start[23]+tar_block)
      : "%esi", "%edx");
 
        if ((t+t0+t1+t2+t3+t4+t5+t6+t7+t8+t9+t10+t11+t12+t13+t14+t15+t16+t17+t18+t19+t20+t21+t22+t23+t24+t25+t26+t27+t28+t29+t30+t31+t32)< 30000 && i>NUM_TEST_HALF*0.3){ 
            sum[j]+=(t+t0+t1+t2+t3+t4+t5+t6+t7+t8+t9+t10+t11+t12+t13+t14+t15+t16+t17+t18+t19+t20+t21+t22+t23+t24+t25+t26+t27+t28+t29+t30+t31+t32);//delta; 
            counter[j]+=1; 
          } 
        if((t+t0+t1+t2+t3+t4+t5+t6+t7+t8+t9+t10+t11+t12+t13+t14+t15+t16+t17+t18+t19+t20+t21+t22+t23+t24+t25+t26+t27+t28+t29+t30+t31+t32)<MAX_CYCLE){
            histogram[j][(t+t0+t1+t2+t3+t4+t5+t6+t7+t8+t9+t10+t11+t12+t13+t14+t15+t16+t17+t18+t19+t20+t21+t22+t23+t24+t25+t26+t27+t28+t29+t30+t31+t32)]++;
          }
          else {
            histogram[j][MAX_CYCLE-1]++; 
        }
        if(j==0){
          arr1[m*NUM_TEST_HALF+i]+=(double)(t+t0+t1+t2+t3+t4+t5+t6+t7+t8+t9+t10+t11+t12+t13+t14+t15+t16+t17+t18+t19+t20+t21+t22+t23+t24+t25+t26+t27+t28+t29+t30+t31+t32);
        } else if(j==1){
          arr2[m*NUM_TEST_HALF+i]+=(double)(t+t0+t1+t2+t3+t4+t5+t6+t7+t8+t9+t10+t11+t12+t13+t14+t15+t16+t17+t18+t19+t20+t21+t22+t23+t24+t25+t26+t27+t28+t29+t30+t31+t32);
        } else if(j==2){
          arr3[m*NUM_TEST_HALF+i]+=(double)(t+t0+t1+t2+t3+t4+t5+t6+t7+t8+t9+t10+t11+t12+t13+t14+t15+t16+t17+t18+t19+t20+t21+t22+t23+t24+t25+t26+t27+t28+t29+t30+t31+t32);
        }
				if(round_rand==0)
		        {
		        	if(j==0){
			          arr4[m*NUM_TEST_HALF+i]+=(double)(t_r+t_r0+t_r1+t_r2+t_r3+t_r4+t_r5+t_r6+t_r7+t_r8);
			        } else if(j==1){
			          arr5[m*NUM_TEST_HALF+i]+=(double)(t_r+t_r0+t_r1+t_r2+t_r3+t_r4+t_r5+t_r6+t_r7+t_r8);
			        } else if(j==2){
			          arr6[m*NUM_TEST_HALF+i]+=(double)(t_r+t_r0+t_r1+t_r2+t_r3+t_r4+t_r5+t_r6+t_r7+t_r8);
			        }
		        } else {
		        	if(j==0){
			          arr4[m*NUM_TEST_HALF+i]+=(double)(t_r+t_r0+t_r1+t_r2+t_r3+t_r4+t_r5+t_r6+t_r7+t_r8-arr1[m*NUM_TEST_HALF+i]);
			        } else if(j==1){
			          arr5[m*NUM_TEST_HALF+i]+=(double)(t_r+t_r0+t_r1+t_r2+t_r3+t_r4+t_r5+t_r6+t_r7+t_r8-arr2[m*NUM_TEST_HALF+i]);
			        } else if(j==2){
			          arr6[m*NUM_TEST_HALF+i]+=(double)(t_r+t_r0+t_r1+t_r2+t_r3+t_r4+t_r5+t_r6+t_r7+t_r8-arr3[m*NUM_TEST_HALF+i]);
			        }
		        }
        //if (j==1 && i==299 && m==30) printf("sched_getcpu_in_step_3 = %d\n", sched_getcpu()); 
        maintain_arr[5000]=STEP0_RUN;
  		} 
  	} 
  } 
  } 
 printf("%lu %lu\n", result[0], result[1]);
 if (2*result[0]>result[1])
   {
   }
     printf("cycles\ta\ta_alias\tNIB\n");
     for(int i=0;i<MAX_CYCLE;i++){
           printf("%d\t%d\t%d\t%d\n", i, histogram[0][i], histogram[1][i], histogram[2][i]);   
     }
     
      double pvalue_a_a_alias = Pvalue(arr1,NUM_TEST_HALF*EACH_RUN,arr2,NUM_TEST_HALF*EACH_RUN);
      double pvalue_a_NIB = Pvalue(arr1,NUM_TEST_HALF*EACH_RUN,arr3,NUM_TEST_HALF*EACH_RUN);
      double pvalue_a_alias_NIB = Pvalue(arr2,NUM_TEST_HALF*EACH_RUN,arr3,NUM_TEST_HALF*EACH_RUN);
     
      double pvalue_a_a_alias_2 = Pvalue(arr4,NUM_TEST_HALF*EACH_RUN,arr5,NUM_TEST_HALF*EACH_RUN);
      double pvalue_a_NIB_2 = Pvalue(arr4,NUM_TEST_HALF*EACH_RUN,arr6,NUM_TEST_HALF*EACH_RUN);
      double pvalue_a_alias_NIB_2 = Pvalue(arr5,NUM_TEST_HALF*EACH_RUN,arr6,NUM_TEST_HALF*EACH_RUN);
     
      fprintf(fp, "1450\tATT_D\tVIC_U\tATT_D\n");
      fprintf(fp, "%f\t%f\t%f\t%f\t%f\t%f\n", pvalue_a_a_alias, pvalue_a_NIB, pvalue_a_alias_NIB, pvalue_a_a_alias_2, pvalue_a_NIB_2, pvalue_a_alias_NIB_2);
     
    
    if (((pvalue_a_a_alias<0.0005 && pvalue_a_NIB<0.0005)||(pvalue_a_a_alias<0.0005 && pvalue_a_alias_NIB<0.0005)||(pvalue_a_alias_NIB<0.0005 && pvalue_a_NIB<0.0005))&&((!u_last_step) ||((pvalue_a_a_alias_2<0.0005 && pvalue_a_NIB_2<0.0005)||(pvalue_a_a_alias_2<0.0005 && pvalue_a_alias_NIB_2<0.0005)||(pvalue_a_alias_NIB_2<0.0005 && pvalue_a_NIB_2<0.0005)))) 
   {
     fprintf(fp, "gen_output/42_AccOne0_AccTwo1_AccThr1_Aff0_plain_1450.c Succeeds \n\n");
     fprintf(fp_res, "1\n");
   } else {
     //
     fprintf(fp_res, "0\n");
   }
  fclose(fp);  
  fclose(fp_res);  
     exit(0);
 }
 // local victim
 if ((pid_l_2 = fork()) < 0) {
     printf("Failed to fork process 2\n");
     exit(1);
 }
 else if (pid_l_2 == 0) {
          CPU_ZERO(&mycpuset); 
          CPU_SET(1, &mycpuset); 
          //set processor affinity 
  	if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &mycpuset) == -1) {
        perror("sched_setaffinity");
    }
 
 for (int m = 0; m < EACH_RUN; m++) 
 { 
  for (int e = 0; e < PROBE_SIZE; ++e)
   {
     sum[e] = 0;
     counter[e] = 0;
   }
    //probe = probe + 0x10*j; 
    for (int i = 0; i < NUM_TEST_HALF; ++i) 
    { 
    for (int round_rand = 0; round_rand < 2; ++round_rand) 
    	{ 
  	for (int j = 0; j < PROBE_SIZE; ++j) 
  		{ 
           
           while(maintain_arr[5000]!=STEP2_RUN)sched_yield(); 
 
          //if (j==1 && i==299 && m==30) printf("sched_getcpu_in_step_2 = %d\n", sched_getcpu()); 
          maintain_arr[5000] = STEP3_RUN;
    }}	 
  }}	 
     exit(0);
 }
 // remote attacker
 if ((pid_r_1 = fork()) < 0) {
     printf("Failed to fork process 3\n");
     exit(1);
 }
 else if (pid_r_1 == 0) {
          CPU_ZERO(&mycpuset); 
          CPU_SET(2, &mycpuset); 
          //set processor affinity 
  	if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &mycpuset) == -1) {
        perror("sched_setaffinity");
    }
 
 for (int m = 0; m < EACH_RUN; m++) 
 { 
  for (int e = 0; e < PROBE_SIZE; ++e)
   {
     sum[e] = 0;
     counter[e] = 0;
   }
    //probe = probe + 0x10*j; 
    for (int i = 0; i < NUM_TEST_HALF; ++i) 
    { 
    for (int round_rand = 0; round_rand < 2; ++round_rand) 
    	{ 
  	for (int j = 0; j < PROBE_SIZE; ++j) 
  		{ 
           
    		}}	 
       }} 
     exit(0);
 }
 // remote victim
 if ((pid_r_2 = fork()) < 0) {
     printf("Failed to fork process 4\n");
     exit(1);
 }
 else if (pid_r_2 == 0) {
          CPU_ZERO(&mycpuset); 
          CPU_SET(3, &mycpuset); 
          //set processor affinity 
  	if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &mycpuset) == -1) {
        perror("sched_setaffinity");
    }
 
 for (int m = 0; m < EACH_RUN; m++) 
 { 
  for (int e = 0; e < PROBE_SIZE; ++e)
   {
     sum[e] = 0;
     counter[e] = 0;
   }
    //probe = probe + 0x10*j; 
    for (int i = 0; i < NUM_TEST_HALF; ++i) 
    { 
    for (int round_rand = 0; round_rand < 2; ++round_rand) 
    	{ 
  	for (int j = 0; j < PROBE_SIZE; ++j) 
  		{ 
           
    }}	 
  }}	 
     exit(0);
 }
} 
