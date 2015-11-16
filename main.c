/* File.........: ht-malloc.c - binary twin memory allocation for embedded systems
 * Author.......: Henrik Thomsen <heth@mercantec.dk>
 * Documentation: http://mars.tekkom.dk/----
 * Source.......: http://github....
 * Standard.....: C99 complient
 *
 * Compiled without warning with GCC V4.8.2. (gcc -std=c99 -pedantic)
 * 
 * SPECS:
 * Binary twin memory allocator using 2^n freelist. Total amount of 
 * memory can be any size.
 **************************************************************************
 *                        W O R K   I N   P R O G R E S S                 *
 **************************************************************************
 *
 * INTERNAL DATASTRUCTS
 * Two data structs make up the allocator. The pool-struct and the 
 * freelist. Both are initialized by mem_init() 
 * pool-struct: Is an array of struct's describing the freelist
 * Example of an initialized pool-struct array: DATAWIDTH=16
 * mem_init is called with <heapsize>i = 2000 and <minsize> = 16
 *
 *  Index...:       [0]     [1]     [2]     [3]     [4]     [5]
 *  Size....:       16      32      64      128     256     512
 *  Offset..:       0       8       12      14      15      16
 *  Avail...:       125     62      31      15      7       3
 *  Fbcou...:       1       0       1       2       2       2
 *  Alloccou:       0       0       0       1       0       0
 *  
 * From the pool structure above it can be seen that there by example
 * are 31 available 64 bytes block available and that 0 are allocated yet.
 * The <fbcou> or free-buddy counter is 1, indicating there are one free buddy
 *
 * in the free-list. If you notice the free-list below you will see there are
 * 31 bits=0 in the size=64 byte blocks in the free list.
 * mem_init also initializes the freelist:
 *
 *  Size  ADR VAL  ADR VAL  ADR VAL  ADR VAL  ADR VAL  ADR VAL  ADR VAL  ADR VAL
 *  0016: 000:0000 001:0000 002:0000 003:0000 004:0000 005:0000 006:0000 007:e000
 *  0032: 008:0000 009:0000 010:0000 011:c000
 *  0064: 012:0000 013:8000
 *  0128: 014:8001
 *  0256: 015:ff81
 *  0512: 016:fff9
 *
 ***************************************************************************
 License:  Free open software but WITHOUT ANY WARRANTY.
 Terms..:  see http://www.gnu.org/licenses
 **************************************************************************/

#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include "ht_malloc.h"

////////////////////////// TESTING ////////////////////////////
#define HEAPSIZE 4096   // Size of heap memory
#define MINSIZE  16      // Minumiim size in bytes to be allocatd
double time_pair, time_buddy; //DEBUG
 clock_t start_pair,start_buddy; //DEBUG
 uint counter; // DEBUG

void printfreelist( void ) {
	int i,j;
	for (i=0; pool[i].size != 0; i++) {
		printf("%04d: ", pool[i].size);
		for (j=0; j < pool[i].avail;j+=DATAWIDTH) {
			//printf("%04x ", freelist[ pool[i].offset + j/8 ]);
			printf("%03d:%04x ",  pool[i].offset + j/DATAWIDTH, freelist[ pool[i].offset + j/DATAWIDTH ] );
		}
		printf("\n");
	}
}
void printpooldesc( void ) {
	int i;
	printf("\n\nIndex...:\t");
	for ( i = 0; pool[i].size != 0 ; i++ ) {
	  printf("[%d]\t",i);
	}
	printf("\nSize....:\t");
	for ( i = 0; pool[i].size != 0 ; i++ ) {
	  printf("%d\t",pool[i].size);
	}
	printf("\nOffset..:\t");
	for ( i = 0; pool[i].size != 0 ; i++ ) {
	  printf("%d\t",pool[i].offset);
	}
	printf("\nAvail...:\t");
	for ( i = 0; pool[i].size != 0 ; i++ ) {
	  printf("%d\t",pool[i].avail);
	}
	printf("\nFbcou...:\t");
	for ( i = 0; pool[i].size != 0 ; i++ ) {
	  printf("%d\t",pool[i].fbcou);
	}
	printf("\nAlloccou:\t");
	for ( i = 0; pool[i].size != 0 ; i++ ) {
	  printf("%d\t",pool[i].alloccou);
	}
	printf("\n");
}


//#define MEMSIZE 6710886
#define MEMSIZE 2000
int main( void ) {
int i,j;
uint a[3];
clock_t start, end;
clock_t total_start, total_end;
clock_t t;
double cpu_time_used;
     
char *buf = (char *) malloc( MEMSIZE );
char clr[] = {0x1b,'[','2' ,'J',0,0x1b,'[','H'};
double *arr[1000000];
void *mem;
time_pair=0;
time_buddy=0;
counter=0;
//#define MEMSIZE 65535*1024
if ( (i=mem_init( MEMSIZE, (uint8 *) buf, MINSIZE) ) == 0 ) {
//if ( i== 0 ) {
	printf("mem_init failed\n");
	return(0);
}
		printfreelist();
//HSH
printf("Datasize (uint) is %d heapstart %x end address %x\n", (unsigned int) sizeof(uint),  (int) (long int) heapstart,
				(int) (long int) heapstart + MEMSIZE);
printf("Datasize (a) is %d\n", (int) sizeof(a) );
printf("Size of actual used memory is %d bytes out of %d (%.2f percent)\n",i,MEMSIZE*10, (float) i / (MEMSIZE*10/100));
printpooldesc();
//printfreelist();
j=0;
printf("========================== BEFORE =====================\n");
total_start = clock();
start = clock();
for ( i = 0; i < 130000 ; i++ ) {
	if ( (i % 10000) == 0 ) {
	end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	time_buddy = time_buddy / CLOCKS_PER_SEC;
printf("\rAllocation %06d size %07d time %f buddy counter %d (%f percentage %f) (address %d and %d)\n",i,i*2+1,cpu_time_used,
       counter, time_buddy, (float) time_buddy / (cpu_time_used/100) , (int) (long int) arr[i-1],(int) (long int) arr[i-2]);

	time_buddy=0;
	time_pair=0;
	counter=0;
	
	start = clock();
	}
	t = clock();
	arr[i] = mem_alloc(16);
	printf("Address: %p\n", (void *) arr[i] );
	j=j+(2);
	if ( i == 52) {
		printf("\ni=%d!!!!!!!!!!!!\n",i);
		printpooldesc();	
		printfreelist();
	}
	if ( arr[i] == 0) {
		printf("\nAllocation returned 0 i=%d!!!!!!!!!!!!\n",i);
		printpooldesc();	
		printfreelist();
		break;
	}
	*arr[i] = i;
}
total_end = clock();

cpu_time_used = ((double) (total_end - total_start)) / CLOCKS_PER_SEC;
printf("Time total: %f gennemsnit: %f\n", cpu_time_used, cpu_time_used/1000000);
printf("TOTAL: %d\n",j);
printf("\n");

printpooldesc();
printfreelist();
printf("================================ i = %d ========================================\n",i);

start = clock();
j=i;
for ( ; i > 0 ; i=i-2 ) {
	if ( (i % 10000) == 0 ) {
	end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("\rFreeing %06d size %07d time %f\n",i,i*2+1,cpu_time_used);
	start = clock();
	}
if (  *arr[i-1] != (double)i-1) {
	printf("SAVED DATA WRONG.... should be %d are %d\n", (int) *arr[i-1], i-1);
}
mem_free( arr[i-1] );
}
printf("TOTAL: %d\n",j);
printfreelist();
printpooldesc();
j--;
for (i=j ; i > 0 ; i=i-2 ) {
	if ( ((i+1) % 10000) == 0 ) {
	end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("\rFreeing %06d size %07d time %f\n",i,i*2+1,cpu_time_used);
	start = clock();
	}
mem_free( arr[i-1] );
}
printf("TOTAL: %d\n",j);
printpooldesc();
free( buf );
return 1;
}
