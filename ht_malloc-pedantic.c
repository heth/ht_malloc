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
	uint	alloccou;	// Number of allocations
	uint	fbcou;	// Free buddy count. 0=No free buddies > 0 number of free buddies
								// Used to reduce allocation processing time, by avoiding looking
								// for free buddies when there are none.
};
typedef struct pd pooldesc;
// Global variables
void *heapstart; // Start of heap-memory to allocate from
pooldesc *pool;
uint  *freelist;

////////////////////////////////// UTILITY FUNCTIONS //////////////////////////
// Calculate power of 2 for number.
// Example: <number>=8 - returns 2^8 = 256
uint power_of_two( uint number ) {
  if (number > DATAWIDTH ) {
    return( 0 );  // Power of <number> is to big for datatype
  }
  else return( 0x1 << number);
}

// Exponent of two. Returns number of times two is
// multiplied to itself to give numner 2^n
// Returns 0 if number is not a power of 2
uint exp_of_2( uint number) {
  uint base;
  uint exponent;
  for (base = 2, exponent = 1; base < number; base*=2, exponent++ );

  if ( base != number ) {
    return( 0 ); // Number NOT a power of 2
  }
  else return exponent;
}
// Function: fill_bits_in_array
// Abstract: To insert <number_bits> of zeros or ones in - remaining bits in last
// Abstract: Fill an array with <number_bits> in <state> - remaining bits in last
//           arrayelement filled with inverted <state> (<state> = 0 | 1)
// Example : If <number_bits> = 67 and <state> = 0 and uint size = 32 bits and
//           <*array[0]> = 0000 0000 0000 0000 0000 0000 0000 0000 : 32 bits = 0
//           <*array[1]> = 0000 0000 0000 0000 0000 0000 0000 0000 : 32 bits = 0
//           <*array[2]> = 1111 1111 1111 1111 1111 1111 1111 1000 :  3 bits = 0
//                               Number of continious bits equal 0 = 67
// Returns number of elements in array used
// <arraysize> is in elements of uint size
// CAUTION!! Arraysize should be greather og equal to DATAWIDTH/<number_bits>
uint fill_bits_in_array( uint *array, uint number_bits, uint state ) {
  uint i;
  for( i=0; number_bits > 0; i++ ) {
    if (number_bits >= DATAWIDTH) {
      array[i] = -state;
      number_bits-=DATAWIDTH;
    } else if ( number_bits > 0 ) {  // Remaining bits
      array[i] = power_of_two( number_bits ) - 1;
      //array[i] = array[i] << DATAWIDTH - number_bits;
      number_bits = 0;  // Just used the last bits
      if (state == 0) {
        array[i] = ~array[i];
      }
    } else {
      array[i] = ~ -state;
    }
  }
  return(i);
}
	
uint inverse( uint org ) {
	uint i;
	i = ( (org & MASK55) << 1) | ( (org & MASKaa) >> 1 );
	return i;
}

inline uint freebinary( uint org ) {
	uint i;
	i = ( (org & MASK55) << 1) | ( (org & MASKaa) >> 1 );
	return i & ~org;
}
// Function: fl_bit_set (Freelist bit set)
// Abstract: Set a specific bit in a array to "1"
// <freelist> is a pointer to an array of unsigned dattypes
// with DATAWIDTH size in bits
// <bitnr> Bitnumber to set
// Example: bitnr = 68 DATAWIDTH=32 -- then bit number 4 in uint
//          number 3 would be set. (fl[2] = fl[2] | 0x8)
// returns nothing
void fl_bit_set( uint *fl, uint bitnr ) {
	uint i;
	// Using rightshift (>>) instead of divide. (DATAWIDTH must be power of 2)
	i = bitnr >> DATAWIDTH_EXPONENT;	// Find arraymember to set bit in
	fl[i] = fl[i] | 1 << ( (bitnr-1) % DATAWIDTH);	
}

// Function: fl_bit_reset (Freelist bit reset)
// Abstract: Set a specific bit in a array to "0"
// <freelist> is a pointer to an array of unsigned dattypes
// with DATAWIDTH size in bits
// <bitnr> Bitnumber to reset
// returns nothing
void fl_bit_reset( uint *fl, uint bitnr ) {
	uint i;
	// Using rightshift (>>) instead of divide. (DATAWIDTH must be power of 2)
	i = bitnr >> DATAWIDTH_EXPONENT;	// Find arraymember to set bit in
	fl[i] = fl[i] & ~(1 << ( (bitnr-1) % DATAWIDTH));	
}
// Function: fl_bit_state (Freelist bit state)
// Abstract: Set a specific bit in a array to "0"
// <freelist> is a pointer to an array of unsigned dattypes
// with DATAWIDTH size in bits
// <bitnr> Bitnumber to reset
// returns state of bitnr 0 if <bitnr>=0 or greather than 0 if <bitnr>=1
uint fl_bit_state( uint *fl, uint bitnr ) {
	uint i;
	// Using rightshift (>>) instead of divide. (DATAWIDTH must be power of 2)
	i = bitnr >> DATAWIDTH_EXPONENT;	// Find arraymember to set bit in
	return( fl[i] & ( 1 << ((bitnr) % DATAWIDTH)) );
}
// Function: fl_find_buddy
// Abstract: Find a free buddy in a binary buddy list and reserve the slot.
// <freelist> is a pointer to an array of unsigned dattypes
// with DATAWIDTH size in bits1
// <avail> is number of available slots counted in bits
// Returns 0 if no binary bodies found and bitnumber if found. The binary buddy
// is reserved by setting the bit to "1"
 int fl_find_buddy( uint *fl, uint avail) {
   uint cnt,i;
   uint bitnr, mask;
   // Traverse array testing each member..
	cnt = avail / DATAWIDTH;
	if ( (avail % DATAWIDTH) != 0) {	// If heapsize not fiting 2^N
  	cnt++;			// Last bits in freelist.
	}
	do {
	 	cnt--;
		mask = ( ( (fl[cnt] & MASK55) << 1) | ( (fl[cnt] & MASKaa) >> 1 ) ) & ~fl[cnt];
    if (mask  != 0 ) {
       // Yes there are - find the first free buddy
       bitnr=1; // Bit numbers are from 1 to DATAWIDTH

       // Use an approximation method to find first free binary buddy.
       // Example: DATAWIDTH=32 Test lower 16 bits then 8 then 4 then 2 and find it
       for ( i = DATAWIDTH>>1 ; i != 0; i=i>>1) {
         // Test if there are any free binaries in the lower half
         if ( (((1 << i) - 1) & mask) == 0) {
          // There is no bits set in lower half - rotate lower half out
           bitnr+=i;  // Bit set in upper half
           mask = mask >> i;
         }
       }
       // reserve the block by setting the until now free binary buddy
       fl[cnt] = fl[cnt] | ( 1 << (bitnr-1));
       return( bitnr + cnt*DATAWIDTH);
     }
   } while ( cnt > 0);
   printf("NO FUCKING BUDDY PRESENT.....\n");
   return(0);
 }

////////////////////////////////// PUBLIC FUNCTIONS //////////////////////////
// Function: mem_init
// Status  : public
// Abstract: Initialize structures for malloc()/new() memory allocator
// Input:
//  <heapsize> - The size of RAM in bytes the allocator can allocate
//               Size must be greather than <minsize>
//  <heap>     - Start address of RAM size if <heapsize>
//  <minsize>  - Minumium size to be allocated. Must be a power of 2
//							 Example: 2,4,8,16.....
// Returns		 - Number of bytes used from <*heap>
uint mem_init(uint heapsize, uint8 *heap, uint minsize) {
	uint i,j;
	uint offsetcou;
	
	// The initialization prepares two data structures, which are reservered in the
	// heap.
	// Step 1: Build pool-structure describing the binary-twin allocation system
	//         Clear the binary-twin freelist
	// Step 3: Allocate the pool-structure and the freelist in the freelist
	heapstart = heap;		// Initialise global heap start pointer
	// Heapsize must be bigger or equal to minsize
	if (  heapsize < minsize ) {
		return(0);	// Error - heapsize too small
	}
	if ( i = exp_of_2( minsize) == 0 ) {
		return(0); // Error - minsize not a power of 2
	}
	// Allocate pooldescriptor in heap start (reserved later)

	// Step 1: Build pool-structure describing the binary-twin allocation system
	pool = (pooldesc *) heap;
	// Fill pool structure. Use minsize as variable to increase alloc-size
	for ( i = 0, offsetcou = 0; minsize*2 <= heapsize; i++, minsize*=2 ) {
		pool[i].size 		= minsize;		// Size of allocation chunk
		pool[i].offset 	= offsetcou;	// Freelist array member where <size> freelist starts
		pool[i].avail		= heapsize / minsize;	// Number of available chunks of minsize bytes
		pool[i].fbcou		= 0;					// No free buddies available
		if ( pool[i].avail % 2 != 0 ) {	// Any free buddies?
			// If uneven number of available buddies, there are one free buddy
			pool[i].fbcou=1;
		}
		// Calculate  how many uint's used in array for freelist
		offsetcou = offsetcou + pool[i].avail / DATAWIDTH;
		if ( (j = pool[i].avail % DATAWIDTH) != 0 )	{ // if avail not a hole fraction of DATAWIDTH
			offsetcou +=1; 		// Last arraymember describe last bits in freelist
		}
	}
	// End of pool descriptor. Zero terminate it.
	// No reason for pool structure for one bit. (top level)
	pool[i].size 		= 0;
	pool[i].offset 	= 0;
	pool[i].avail 	= 0;
	pool[i].fbcou 	= 1;	// Set to odd number to simplify allocation of memory
	pool[i].alloccou= 0;

	// Calculate beginning of freelist rigth after pool structure		 
	freelist =  (uint *) &pool[i+1].size;	// Freelist begin after poll structrure
	// Initialize freelist with information from pool structure
	for ( i = 0; pool[i].size != 0; i++ ) {
		j = fill_bits_in_array( &freelist[ pool[i].offset ], pool[i].avail,0); // Set all chunks free
	}
	// Exiting this loop i = zero terminated pool struct entry. j = number of array elements used
  // by fill_bits_in_array in freelist.
	//Calculate size of poll-structure + freelist - The used memory must be reserved in freelist!
	offsetcou = sizeof(*pool)* (i+1);	// First calculate size of pool structure
	offsetcou = offsetcou + pool[i-1].offset *sizeof(uint) + j * sizeof(uint);	// Then add freelist size
	//offsetcou=offsetcou*2;
	// Now reserve memory used for pool structure and freelist
	for( i = 1, j = 0; pool[j].size != 0 ; j++) {
		if ( offsetcou <= pool[j].size ) {
			//HETHfreelist[ pool[j].offset] = freelist[ pool[j].offset] | 1 << (DATAWIDTH - 1);
			freelist[ pool[j].offset] = freelist[ pool[j].offset] | 1;
			//if ( pool[j].avail != 3 ) {	// Only one fbcou free - when two reserved.
				pool[j].fbcou+=1;		// One free buddy available, when reserving one block.
			//}
			pool[j].alloccou = i;
			i=0;	// <alloccou> only counted up on the first - setting i=0 avoid remaining..
		}
	}			
	return(offsetcou);
}
// Function: mem_rmalloc()
// Abstract: Resilient malloc. Use rmalloc() for datastructures that have a long life.
//           rmalloc() allocates memory from begginning of the heap and normal malloc
//           allocates from the end of the heap. Using normal malloc for transient 
//           datastructures will help preserve as big blocks as possible.
void *mem_alloc( uint16 size ) {
	uint size_match;	// Pointer in pool to the size matching the wanted <size>
	uint buddy; 
	uint i,j;
	for ( size_match = 0; pool[size_match].size < size & pool[size_match].size != 0; size_match++);
	if ( pool[size_match].size == 0 ) {	// Requested size too big
		return(0);
	}

	// Are there a free buddy?
	if ( pool[size_match].fbcou > 0 ) {
		buddy = fl_find_buddy( (uint *) freelist + pool[size_match].offset, pool[size_match].avail);
		pool[size_match].fbcou--;		// One less buddy 
		pool[size_match].alloccou++;// One more allocation of this size
		return( (uint8 *) heapstart + pool[size_match].size * (buddy-1) );
	}
	// No free buddy found. Need to find bigger block to divide info preferred size.
	// There will always exist a binary buddy on some level,
  // after first allocation made by mem_init()

	// When initializing pool structure, the last struct is zero-terminated in <pool[last].size>
  // and <pool[last].fbcou> is set to "1"
	for ( i = size_match + 1; pool[i].fbcou == 0; i++);
	
	if ( pool[i].size == 0 ) {
		return(0);	// Allocation impossible - no free memory at or above requested size.
	}
	// Free binary found on some upper level - reserve the block using fl_find_buddy()
	buddy = fl_find_buddy( (uint *) freelist + pool[i].offset, pool[i].avail);
	pool[i].fbcou--;	// Used one free buddy

	// Free buddy found - Allocate buddies down to size allocated
	// Example: If caller requested 128 byte and there are no free binary buddies in
	//					the 128,256 and 512 bytes freelist but there are one in the 1024 bytes freelist
	//					Then allocate the 1024 byte block and divide in two 512 bytes block. Divide one
	//					of the 512 in two 256 and one 256 into two 128 of which one is allocated for the
	//					caller.
	//------
	// The variable <i> contain the pool entry with a free binary buddy. (In example <i> is 1024)
	// The variable <buddy> contains the number of the 1024 byte block. Starting with 1
	// The <buddy> bit doubles for each level. (Example: buddy= bit 7, 512=bit 14, 256=bit 28..)
	// Now allocate binary. (In example allocate one 512 then one 256 and last one 128 byte)
	for ( i-- ; pool[i].size >= pool[size_match].size ; i-- ) {
		// calculate bit in freelist that should be reserved.
		// Example: If Free list bit 2 was reserved at 1024 bytes block corresponds bit 3 and 4
    //          in 512 byte blocks and bit 5,6,7 and 8 in 256...
		buddy = buddy<<1;
		//fl_bit_set( (uint *) freelist + pool[i].offset, buddy-1);
		buddy-=1;
		fl_bit_set( (uint *) &freelist[pool[i].offset], buddy);
		pool[i].fbcou++;	// One free buddy 
		if ( i == 0) {  // Lowest size done... break loop
			break;
		}
	}
		pool[size_match].alloccou++;	// One free buddy 
	return( (uint8 *) heapstart + pool[size_match].size *( buddy-1) );
}
// function: fl_free_buddy
// Input: <*fl>   Address of freelist
//        <bitnr> Bitnumber to set as free in frellist
// Returns : 0 if the bit freed buddy is 0
//           non-zero if the bit freed buddy is 1
uint fl_free_buddy( uint *fl, uint bitnr ) {
	uint member;
	uint leg;
	
	// Find <fl[]> array member where bit is
	member = bitnr >> DATAWIDTH_EXPONENT;

	// Find bit number in <fl[member]> to be freed
	bitnr = bitnr % DATAWIDTH;
	bitnr++;	
	// Set bitnumber to '0'
	fl[member] = fl[member] & (~( 1 << (bitnr-1)));
	leg = (~( 1 << (bitnr-1)));
	//Check if <bitnr> buddy is set
	if (bitnr % 2 != 0 ) { // If bit even buddy is left bit
		bitnr++ ;
	} else {
		bitnr--; // Else buddy is right bit
	}
	return( fl[member] & (1 << (bitnr-1)) );
}


void mem_free( void *poi ) {
	uint i,j,bitnr;
	// Bit kan være 0 hvis højere oppe i træet. kun lavest i træet skal alloccou--		
	// First find which <pool.size> is allocated. Lowest in binary tree
	for ( i = 0; pool[i].size != 0; i++ ) {
		bitnr = (  ( (uint8 *) poi -  (uint8 *) heapstart ) / pool[i].size);
		if ( (j = fl_bit_state( &freelist[ pool[i].offset], bitnr)) != 0 ) {	
			pool[i].alloccou--;
			break;
		}
	}
	for ( ; pool[i].size != 0; i++ ) {
	// HETH pool[i].size er altid power-of-two
	
		bitnr =  ( (  (uint8 *) poi -  (uint8 *) heapstart ) / pool[i].size);
		if ( fl_free_buddy(&freelist[pool[i].offset], bitnr) != 0) { 
			pool[i].fbcou++;
			// If there is a ocupied buddy - dont free up the binary tree
			return;	// memory block freed for future use
		}
		pool[i].fbcou--;
	}
} 
