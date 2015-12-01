# ht_malloc
Binary pair malloc implementation for embedded systems without MMU unit.
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
