//
//  COMP1927 Assignment 1 - Vlad: the memory allocator
//  allocator.c ... implementation
//
//  Created by Liam O'Connor on 18/07/12.
//  Modified by John Shepherd in August 2014, August 2015
//  Copyright (c) 2012-2015 UNSW. All rights reserved.
//

#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define HEADER_SIZE    sizeof(struct free_list_header)  
#define MAGIC_FREE     0xDEADBEEF
#define MAGIC_ALLOC    0xBEEFDEAD

typedef unsigned char byte;
typedef u_int32_t vlink_t;
typedef u_int32_t vsize_t;
typedef u_int32_t vaddr_t;

typedef struct free_list_header {
   u_int32_t magic;  // ought to contain MAGIC_FREE
   vsize_t size;     // # bytes in this block (including header)
   vlink_t next;     // memory[] index of next free block
   vlink_t prev;     // memory[] index of previous free block
} free_header_t;

// Global data

static byte *memory = NULL;   // pointer to start of allocator memory
static vaddr_t free_list_ptr; // index in memory[] of first block in free list
static vsize_t memory_size;   // number of bytes malloc'd in memory[]


// Input: size - number of bytes to make available to the allocator
// Output: none              
// Precondition: Size is a power of two.
// Postcondition: `size` bytes are now available to the allocator
// 
// (If the allocator is already initialised, this function does nothing,
//  even if it was initialised with different size)

void vlad_init(u_int32_t size)
{
    printf("\n\n");
	if (memory != NULL) {
		abort();
	}

	int i=0;
	for (i=0; 2<<i < size || 2<<i < 512; i++);

	size =(int) 2<<i;


	//malloc memory
	memory = malloc(size);
	if (memory == NULL) {
		
		abort();
	}

	free_list_ptr = 0;
	memory_size = size;

	//create initial header
	free_header_t *initH = (free_header_t *) memory;
	
	initH->magic = MAGIC_FREE;
	initH->size = size;
	initH->next = (vlink_t) 0;
	initH->prev = (vlink_t) 0;
}


// Input: n - number of bytes requested
// Output: p - a pointer, or NULL
// Precondition: n is < size of memory available to the allocator
// Postcondition: If a region of size n or greater cannot be found, p = NULL 
//                Else, p points to a location immediately after a header block
//                      for a newly-allocated region of some size >= 
//                      n + header size.

//Given a region link, create another free header half way though the region.
//Does nothing if there isn't enough space.

//Extra Malloc Functions
static void removeCurrFromFreeList(vlink_t currFreeLink);
static void splitRegions(vlink_t region, size_t totalSize);
static void createHalfRegion (vlink_t region, size_t totalSize);
static vlink_t getNewPrev(vlink_t region);
static vlink_t getBestFit(vsize_t totalSize);


void *vlad_malloc(u_int32_t n)
{

    if (memory==NULL) {

        fprintf(stderr, "You must initialise memory\n");
        return NULL;
    }

    //Must malloc a region > 2^3
    if ((signed int)n < (1<<3)) {

        return NULL;
    }

    vsize_t totalSize = n + HEADER_SIZE;

    int i;
    for (i = 0; totalSize > 2<<i; i++);

    totalSize = 2<<i;
	
	vlink_t currFreeLink = free_list_ptr;
	free_header_t * currFreeHead = (free_header_t *) &memory[currFreeLink];
	
	//one region
	if (currFreeLink == currFreeHead->next) {
		if (currFreeHead->size < totalSize+HEADER_SIZE) {

			return NULL;
        } 
    }

    //general case
    currFreeLink = getBestFit(totalSize);
    currFreeHead = (free_header_t *) &memory[currFreeLink];

        if (currFreeHead->size >= totalSize) {

            splitRegions(currFreeLink, totalSize);
            
            if (currFreeLink == currFreeHead->next) {
                fprintf(stderr, "There is not enough space for a second header\n");

                return NULL;
            }

            currFreeHead->magic = MAGIC_ALLOC;

            removeCurrFromFreeList(currFreeLink);

            free_list_ptr = currFreeHead->next;

            return &memory[currFreeLink+HEADER_SIZE];
        }

    return NULL;
}

//Given a region, creates half regions until the regionsize is as clost to totalSize as possible.
static void splitRegions(vlink_t region, size_t totalSize) {
        free_header_t * currFreeH = (free_header_t *) &memory[region];

        while (currFreeH->size/2 >= totalSize) {

                createHalfRegion(region, totalSize);
        }
    
}

static void createHalfRegion (vlink_t region, size_t totalSize) {
    free_header_t * regionHead =(free_header_t *) &memory[region];
             
    //create new header
    vlink_t newHLink = region + regionHead->size/2;
    free_header_t *newH = (free_header_t *) &memory[newHLink];
    
    newH->magic = MAGIC_FREE;
    newH->size = regionHead->size/2;
    newH->next = (vlink_t) regionHead->next;
    newH->prev = region;
    
    //fix current header
    regionHead->next = newHLink;
    regionHead->prev = getNewPrev(region);
    regionHead->size = regionHead->size/2;

    //fix newH->next prev value
    free_header_t * newHNext =(free_header_t *) &memory[newH->next];
    newHNext->prev = newHLink;
    
}  

static vlink_t getNewPrev(vlink_t region) {

    free_header_t * regionHead =(free_header_t *) &memory[region];
    vlink_t currRegion;

    for(currRegion = region; regionHead->next != region;){
        
        if (regionHead->next == region) {
            break;
        } else {
            currRegion = regionHead->next;
            regionHead = (free_header_t *) &memory[regionHead->next];
        }
    }

    return currRegion;
}

static void removeCurrFromFreeList(vlink_t currFreeLink) {
    
    free_header_t * currFreeHead = (free_header_t *) &memory[currFreeLink];
    free_header_t * currPrevHead = ((free_header_t *) &memory[currFreeHead->prev]);
    free_header_t * currNextHead = ((free_header_t *) &memory[currFreeHead->next]);

    currPrevHead->next = currFreeHead->next;
    currNextHead->prev = currFreeHead->prev;

}

static vlink_t getBestFit(vsize_t totalSize) {
    
    free_header_t * currH = (free_header_t *) &memory[free_list_ptr];
    vlink_t currL = free_list_ptr;

    if (currH->size == totalSize) {

        return currL;
    }

    //search the list for the region with the closet fit
    currL = currH->next;
    currH = (free_header_t *) &memory[currL];

    while (totalSize != currH->size) {

        if (currL == free_list_ptr) {
            totalSize = totalSize<<1;
        
        }

        currL = currH->next;
        currH = (free_header_t *) &memory[currL];
    }
    
    return currL;
}


// Input: object, a pointer.
// Output: none
// Precondition: object points to a location immediately after a header block
//               within the allocator's memory.
// Postcondition: The region pointed to by object can be re-allocated by 
//                vlad_malloc

void alloc2Free(vlink_t node, vlink_t prev, vlink_t next);
void merge(vlink_t curr);

void vlad_free(void *object)
{
    free_header_t * objH = (free_header_t *) (object-HEADER_SIZE);

   if (objH->magic != MAGIC_ALLOC) {
        fprintf(stderr, "xxxxAttemped to free non malloced memory/n/n");
        return;
   }

    free_header_t * currH = (free_header_t *) &memory[free_list_ptr];
    //vlink_t prevL;
    vlink_t nextL;
    vlink_t searchL = free_list_ptr+1;
    vlink_t currL = free_list_ptr;
    
    while (1<2) {
        
        while ((free_header_t * ) &memory[searchL] != (free_header_t * ) &memory[currH->next]) {

            if ((free_header_t *) &memory[searchL] == objH) {

                //prevL = currL;
                nextL = currH->next;

                alloc2Free(searchL, currL, nextL);

                merge(searchL); 

                return;
            }

            if (searchL+1 == memory_size) {
                searchL=0;
            } else {
                searchL++;
            }
        }
        currL = currH->next;
        currH = (free_header_t *) &memory[currL];
        
        if (currH == (free_header_t *) &memory[free_list_ptr]) {
            fprintf(stderr, "No node found... Something went wrong\n");
            break;
        }
    }
}

//Delete current node, set the address to null so It can't be accessed.
void alloc2Free(vlink_t node, vlink_t prev, vlink_t next) {
    
    free_header_t * currH = (free_header_t *) &memory[node];
    free_header_t * prevH = (free_header_t *) &memory[prev];
    free_header_t * nextH = (free_header_t *) &memory[next];

    currH->magic = MAGIC_FREE;  
    currH->prev = prev;
    currH->next = next;

    prevH->next = node;
    nextH->prev = node;

    if (node < prev) {
        free_list_ptr = node;
    }                                 
}

//takes a free link and merges its related regions
void merge(vlink_t curr) {

    free_header_t * currH = (free_header_t *) &memory[curr];
    
    if (currH->magic == MAGIC_ALLOC) {
        fprintf(stderr, "Passed an allocated region\n");
        return;
    }

    free_header_t * prevH = (free_header_t *) &memory[currH->prev];
    free_header_t * nextH = (free_header_t *) &memory[currH->next];

    //Delete curr->next
    if ((curr/currH->size)%2 == 0) {
        if (currH->next-curr == currH->size
            && currH->size == nextH->size 
            && nextH->magic == MAGIC_FREE) {
            
            //fix current header
            currH->next = nextH->next;
            currH->size = currH->size*2;

            //fix new nextH
            free_header_t * newNextH = (free_header_t *) &memory[nextH->next];
            newNextH->prev = curr;

            //delete the old nextH
            nextH->magic = 0;

            //fix free list ptr
            free_list_ptr = curr;

            merge(curr);
        }
    //Delete curr
    } else if ((curr/currH->size)%2 == 1) {
        if (curr-currH->prev == currH->size
            && currH->size == prevH->size 
            && prevH->magic == MAGIC_FREE) {
            
            //fix prev header
            prevH->next = currH->next;
            prevH->size = prevH->size*2;

            //fix newNext
            nextH->prev = currH->prev;

            //delete curr
            currH->magic = 0;

            //fix free list ptr
            free_list_ptr = currH->prev;
            
            merge(currH->prev);
        }
    }

}


// Stop the allocator, so that it can be init'ed again:
// Precondition: allocator memory was once allocated by vlad_init()
// Postcondition: allocator is unusable until vlad_int() executed again

void vlad_end(void)
{
    if (memory != NULL) {
        free_header_t * memoryH = (free_header_t *) memory;
       
    //Check that a genuine memory pointer is being passed in.
       if  (memoryH->magic == MAGIC_FREE 
         || memoryH->magic == MAGIC_ALLOC) {
        
            free(memory);
            
            memory = NULL;
            
            free_list_ptr = 0;
            memory_size = 0;
        }
    }
    
}


// Precondition: allocator has been vlad_init()'d
// Postcondition: allocator stats displayed on stdout

void vlad_stats(void)
{
   
   free_header_t * currFreeHead = (free_header_t *) &memory[free_list_ptr];
    vlink_t currFreeLink = free_list_ptr;

    printf("\n\n-----Testing Free List-----\n");

    //Initial value
    printf("memory[%d]:\n||magic: %d\n||size: %d\n||next: %d\n||prev: %d\n\n", 
           currFreeLink, currFreeHead->magic, currFreeHead->size, currFreeHead->next,
           currFreeHead->prev);
        int temp = currFreeHead->next;
        currFreeHead = (free_header_t *) &memory[currFreeHead->next];

    for(currFreeLink = temp; currFreeLink != free_list_ptr;){
         printf("memory[%d]:\n||magic: %d\n||size: %d\n||next: %d\n||prev: %d\n\n", 
           currFreeLink, currFreeHead->magic, currFreeHead->size, currFreeHead->next,
           currFreeHead->prev);
         
         currFreeLink = currFreeHead->next;
         currFreeHead = (free_header_t *) &memory[currFreeLink];
    }

    printf("---Doing Raw Test---\n");

    //raw search of entire memory for headers
    currFreeHead = (free_header_t *) &memory[0];
    currFreeLink = 0;
    while (currFreeLink < memory_size) {

        if ( currFreeHead->magic == MAGIC_FREE) {
            printf("Free Header: memory[%d]:\n||magic: %d\n||size: %d\n||next: %d\n||prev: %d\n\n", 
           currFreeLink, currFreeHead->magic, currFreeHead->size, currFreeHead->next,
           currFreeHead->prev);

        } else if (currFreeHead->magic == MAGIC_ALLOC) {
            printf("Allocated Header: memory[%d]:\n||magic: %d\n||size: %d\n||next: %d\n||prev: %d\n\n", 
           currFreeLink, currFreeHead->magic, currFreeHead->size, currFreeHead->next,
           currFreeHead->prev);
        }
        currFreeLink++;
         currFreeHead = (free_header_t *) &memory[currFreeLink];
    }


    printf("Finished testing memory\n\n"
           "-------------------------------\n\n");
   return;
}


//
// All of the code below here was written by Alen Bou-Haidar, COMP1927 14s2
//

//
// Fancy allocator stats
// 2D diagram for your allocator.c ... implementation
// 
// Copyright (C) 2014 Alen Bou-Haidar <alencool@gmail.com>
// 
// FancyStat is free software: you can redistribute it and/or modify 
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or 
// (at your option) any later version.
// 
// FancyStat is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>


#include <string.h>

#define STAT_WIDTH  32
#define STAT_HEIGHT 16
#define BG_FREE      "\x1b[48;5;35m" 
#define BG_ALLOC     "\x1b[48;5;39m"
#define FG_FREE      "\x1b[38;5;35m" 
#define FG_ALLOC     "\x1b[38;5;39m"
#define CL_RESET     "\x1b[0m"


typedef struct point {int x, y;} point;

static point offset_to_point(int offset,  int size, int is_end);
static void fill_block(char graph[STAT_HEIGHT][STAT_WIDTH][20], 
                        int offset, char * label);



// Print fancy 2D view of memory
// Note, This is limited to memory_sizes of under 16MB
void vlad_reveal(void *alpha[26])
{
    int i, j;
    vlink_t offset;
    char graph[STAT_HEIGHT][STAT_WIDTH][20];
    char free_sizes[26][32];
    char alloc_sizes[26][32];
    char label[3]; // letters for used memory, numbers for free memory
    int free_count, alloc_count, max_count;
    free_header_t * block;

	// TODO
	// REMOVE these statements when your vlad_malloc() is done

    // initilise size lists
    for (i=0; i<26; i++) {
        free_sizes[i][0]= '\0';
        alloc_sizes[i][0]= '\0';
    }

    // Fill graph with free memory
    offset = 0;
    i = 1;
    free_count = 0;
    while (offset < memory_size){
        block = (free_header_t *)(memory + offset);
        if (block->magic == MAGIC_FREE) {
            snprintf(free_sizes[free_count++], 32, 
                "%d) %d bytes", i, block->size);
            snprintf(label, 3, "%d", i++);
            fill_block(graph, offset,label);
        }
        offset += block->size;
    }

    // Fill graph with allocated memory
    alloc_count = 0;
    for (i=0; i<26; i++) {
        if (alpha[i] != NULL) {
            offset = ((byte *) alpha[i] - (byte *) memory) - HEADER_SIZE;
            block = (free_header_t *)(memory + offset);
            snprintf(alloc_sizes[alloc_count++], 32, 
                "%c) %d bytes", 'a' + i, block->size);
            snprintf(label, 3, "%c", 'a' + i);
            fill_block(graph, offset,label);
        }
    }

    // Print all the memory!
    for (i=0; i<STAT_HEIGHT; i++) {
        for (j=0; j<STAT_WIDTH; j++) {
            printf("%s", graph[i][j]);
        }
        printf("\n");
    }

    //Print table of sizes
    max_count = (free_count > alloc_count)? free_count: alloc_count;
    printf(FG_FREE"%-32s"CL_RESET, "Free");
    if (alloc_count > 0){
        printf(FG_ALLOC"%s\n"CL_RESET, "Allocated");
    } else {
        printf("\n");
    }
    for (i=0; i<max_count;i++) {
        printf("%-32s%s\n", free_sizes[i], alloc_sizes[i]);
    }

}

// Fill block area
static void fill_block(char graph[STAT_HEIGHT][STAT_WIDTH][20], 
                        int offset, char * label)
{
    point start, end;
    free_header_t * block;
    char * color;
    char text[3];
    block = (free_header_t *)(memory + offset);
    start = offset_to_point(offset, memory_size, 0);
    end = offset_to_point(offset + block->size, memory_size, 1);
    color = (block->magic == MAGIC_FREE) ? BG_FREE: BG_ALLOC;

    int x, y;
    for (y=start.y; y < end.y; y++) {
        for (x=start.x; x < end.x; x++) {
            if (x == start.x && y == start.y) {
                // draw top left corner
                snprintf(text, 3, "|%s", label);
            } else if (x == start.x && y == end.y - 1) {
                // draw bottom left corner
                snprintf(text, 3, "|_");
            } else if (y == end.y - 1) {
                // draw bottom border
                snprintf(text, 3, "__");
            } else if (x == start.x) {
                // draw left border
                snprintf(text, 3, "| ");
            } else {
                snprintf(text, 3, "  ");
            }
            sprintf(graph[y][x], "%s%s"CL_RESET, color, text);            
        }
    }
}

// Converts offset to coordinate
static point offset_to_point(int offset,  int size, int is_end)
{
    int pot[2] = {STAT_WIDTH, STAT_HEIGHT}; // potential XY
    int crd[2] = {0};                       // coordinates
    int sign = 1;                           // Adding/Subtracting
    int inY = 0;                            // which axis context
    int curr = size >> 1;                   // first bit to check
    point c;                                // final coordinate
    if (is_end) {
        offset = size - offset;
        crd[0] = STAT_WIDTH;
        crd[1] = STAT_HEIGHT;
        sign = -1;
    }
    while (curr) {
        pot[inY] >>= 1;
        if (curr & offset) {
            crd[inY] += pot[inY]*sign; 
        }
        inY = !inY; // flip which axis to look at
        curr >>= 1; // shift to the right to advance
    }
    c.x = crd[0];
    c.y = crd[1];
    return c;
}