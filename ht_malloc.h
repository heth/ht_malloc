/* File.........: ht_malloc.h - binary twin memory allocation for embedded systems
 * Author.......: Henrik Thomsen <heth@mercantec.dk>
 * Documentation: http://mars.tekkom.dk/----
 * Source.......: http://github....
 * Standard.....: C99 complient
 *
 * See ht_malloc.c for documentation.V4.8.2.
 *
 ***************************************************************************
 License:  Free open software but WITHOUT ANY WARRANTY.
 Terms..:  see http://www.gnu.org/licenses
 **************************************************************************/

// Define datatypes used

typedef unsigned char				uint8;	// Adapt to your  8 bit unsigned datatype 
typedef unsigned short int  uint16; // Adapt to your 16 bit unsigned datatype
typedef unsigned int        uint32; // Adapt to your 32 bit unsigned datatype
typedef unsigned long long  uint64; // Adapt to your 64 bit unsigned datatype

#ifndef DATAWIDTH
	#define DATAWIDTH 16	// DATAWIDTH must be a power of two!!
#endif
#if DATAWIDTH == 16
  typedef uint16 uint;
	#define MASK55	0x5555
	#define MASKaa	0xaaaa
	#define DATAWIDTH_EXPONENT 4 // 2^DATAWIDTH_EXPONENT = DATAWIDTH
#elif DATAWIDTH == 32
  typedef uint32 uint;
	#define MASK55	0x55555555
	#define MASKaa	0xaaaaaaaa
	#define DATAWIDTH_EXPONENT 5
#elif DATAWIDTH == 64
  typedef uint64  uint;
	#define MASK55	0x5555555555555555
	#define MASKaa	0xaaaaaaaaaaaaaaaa
	#define DATAWIDTH_EXPONENT 6
#endif
struct pd {   // heap memory pool descriptor
   uint  size;   // Size of memory block in powers of 2
   uint  offset; // Offset in uint's from beginning of freelist
   uint  avail;  // number of available memory blocks of size
   uint  alloccou; // Number of allocations
   uint  fbcou;  // Free buddy count. 0=No free buddies > 0 number of free buddies
                 // Used to reduce allocation processing time, by avoiding looking
                 // for free buddies when there are none.
 };
 typedef struct pd pooldesc;
 // Global variables
 void *heapstart; // Start of heap-memory to allocate from
 pooldesc *pool;
 uint  *freelist;

