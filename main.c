// Olivia Zino & Layann Wardeh
// ozino, lwardeh 
// Project 5 -- Due April 11 @ 5pm 
// main.c 

/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Global pointer to the disk object */
struct disk *disk = 0;

/* Global pointer to the physical memory spacet */
unsigned char *physmem = 0;

/* Global pointer to the virtual memory spacet */
unsigned char *virtmem = 0;

/* frame_table[frame] = page if full;  -1 if empty */
int *frame_table;

/* global variable to identify the algorithm we should use -- given to us in main() */
const char *algoname;

/* hand of the clock */
int hand = 0; 

/* for clock to compare to */
int* refSeq;

/* to be used in custom alg */
int *frequency;

/* find an available frame using a selected replacement alg */
int freeFrameFinder(struct page_table *pt, int page, int *kicked) { 
	// is there a frame currently free to fill? 
	for (int i = 0; i < page_table_get_nframes(pt); i++) {
		if (frame_table[i] == -1) {
			*kicked = -1; 
			return i; // location of frame to use 
		}
	}
	if (!strcmp(algoname, "rand")) { // started with this because he said to do rand first but then it does not work without the other stuff i think 
        // just pick one & go 
		int replacedFrame = rand() % page_table_get_nframes(pt); // make sure value is an actual frame 
		*kicked = frame_table[replacedFrame];
		return replacedFrame; 

    } else if (!strcmp(algoname, "clock")) {
        // CLOCK logic from class 
		int nframes = page_table_get_nframes(pt);
		while (1) { 
			if (refSeq[hand] == 0) { 
				// kick this frame
				int replacedFrame = hand; 
				*kicked = frame_table[replacedFrame];
				hand = (hand + 1) % nframes; 
				return replacedFrame;
			} else { 
				// check again just in case 
				refSeq[hand] = 0; 
				hand = (hand + 1) % nframes;
			}
		}

    } else if (!strcmp(algoname, "custom")) {
        // what we have come up with = using frequency as a tiebreaker for CLOCK
		int nframes = page_table_get_nframes(pt);
    int start = hand;
    int candidate = -1;
    int lowestFreq = __INT_MAX__;

    while (1) {
        if (refSeq[hand] == 0) {
            int page = frame_table[hand];
            if (frequency[hand] < lowestFreq) {
                candidate = hand;
                lowestFreq = frequency[hand];
            }
        } else {
            refSeq[hand] = 0;
        }
        hand = (hand + 1) % nframes;
       // we have returned back to 00:00 
        if (hand == start) {
            break;
        }
    }
    if (candidate != -1) {
        *kicked = frame_table[candidate];
        hand = (candidate + 1) % nframes;
        return candidate;
    }

    // if all had 1 --> just use hand
    *kicked = frame_table[hand];
    int victim = hand;
    hand = (hand + 1) % nframes;
    return victim;
	}
}
/* A dummy page fault handler to start.  This is where most of your work goes. */
void page_fault_handler(struct page_table *pt, int page )
{
	/* // try it out portion -- page N maps directly to frame N: 
	page_table_set_entry(pt, page, page, BIT_PRESENT | BIT_WRITE); */
	int kicked = -1;
	int frame = freeFrameFinder(pt, page, &kicked);
	page_table_set_entry(pt, page, frame, BIT_PRESENT);
	/* original dummy code
	printf("page fault on page #%d\n",page);
	exit(1); */

	// actual attempt: 
	// int frame; 
	//int bits; 
	//page_table_get_entry(pt, page, &frame, &bits);

	/* CASE 1 -- PAGE IS PRESENT - WRITE FAULT OCCURS */

	/* CASE 2 -- PAGE IS NOT PRESENT AND WE NEED TO FIND A FRAME */
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|clock|custom> <alpha|beta|gamma|delta>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	algoname = argv[3];
	const char *program = argv[4];

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	physmem = page_table_get_physmem(pt);
	virtmem = page_table_get_virtmem(pt);

	/* data structures for replacement algs */
	frame_table = malloc(sizeof(int) * nframes);
	refSeq = malloc(sizeof(int) * nframes);
	frequency = malloc(sizeof(int) * nframes);

	for (int i = 0; i < nframes; i++) {
		frame_table[i] = -1;
		refSeq[i] = 0;
		frequency[i] = 0;
	}
	
	if(!strcmp(program,"alpha")) {
		alpha_program(pt,virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"beta")) {
		beta_program(pt,virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"gamma")) {
		gamma_program(pt,virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"delta")) {
		delta_program(pt,virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[4]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
