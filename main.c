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

// using for testing performance of algs 
# include <time.h> 

struct disk *disk      = 0; // Global pointer to the disk object 
unsigned char *physmem = 0; // Global pointer to the physical memory spacet
unsigned char *virtmem = 0; // Global pointer to the virtual memory spacet

int *frame_table; 			// frame_table[frame] = page if full;  -1 if empty
const char *algoname; 		// global variable to identify the algorithm we should use -- given to us in main()

int hand = 0; 	// hand of the clock 
int* refSeq; 	// for clock to compare to
int *frequency; // to be used in custom alg 

/* stats variables */ 
int faultCounter = 0; 
int reads2disk   = 0; 
int writes2disk  = 0; 
 

// find an available frame using a selected replacement alg 
int freeFrameFinder(struct page_table *pt, int page, int *kicked) { 
	// is there a frame currently free to fill? 
	int nframes = page_table_get_nframes(pt); 

	for (int i = 0; i < nframes; i++) {
		if (frame_table[i] == -1) {
			*kicked = -1; 
			return i; 	// location of frame to use 
		}
	}
	if (!strcmp(algoname, "rand")) { // started with this because he said to do rand first but then it does not work without the other stuff i think 
        // just pick one & go 
		int replacedFrame; 
		do {
			replacedFrame = rand() % nframes; // make sure value is an actual frame 
		} while (frame_table[replacedFrame] == -1);
		
		*kicked = frame_table[replacedFrame];
		return replacedFrame; 

    } else if (!strcmp(algoname, "clock")) {
        // CLOCK logic from class 
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
        // what we have come up with = using frequency as an enhancer for CLOCK
		int nframes = page_table_get_nframes(pt);
		int candidate = -1;
		int currentBestChoice = __INT_MAX__;

		for (int i =0; i < nframes; i++) {
			// lower freq = less used 
			int score = frequency[i];
			// if it was recently used, make sure it does not get replaced by heavily weighting it 
			if (refSeq[i]) {
				score = score + 250; // 250 is a voodoo constant just to bump it up 
			} else {
				score = score + 0;
			}

			if (score < currentBestChoice) { 
				currentBestChoice = score; 
				candidate = i; // this will be removed 
			}
		}

		// choose the page to replace & advance clock hand 
		*kicked = frame_table[candidate];
		hand = (candidate + 1) % nframes;
		return candidate;
	}

	// handle user error in command line 
	fprintf(stderr, "Unknown algorithm: %s\n", algoname);
    exit(1);
}

/* A dummy page fault handler to start.  This is where most of your work goes. */
void page_fault_handler(struct page_table *pt, int page ) {
	/* // try it out portion -- page N maps directly to frame N: 
	page_table_set_entry(pt, page, page, BIT_PRESENT | BIT_WRITE); */

	/* original dummy code
	printf("page fault on page #%d\n",page);
	exit(1); */

	int frame; 
	int bits; 
	page_table_get_entry(pt, page, &frame, &bits);
	faultCounter++;
	
	/* CASE 1 -- PAGE IS PRESENT - WRITE FAULT OCCURS */
	if (bits & BIT_PRESENT) {
		page_table_set_entry(pt, page, frame, bits | BIT_WRITE);
		refSeq[frame] = 1;
		frequency[frame]++; // update that it has been used 
		return;
	}
	

	/* CASE 2 -- PAGE IS NOT PRESENT -  NEED TO LOAD IT */
	// bounce if things need to go 
	int kicked_page;
	int replacement_frame = freeFrameFinder(pt, page, &kicked_page);
		
		// DEBUGGING 
		// printf("FAULT: loading page %d into frame %d, evicting page %d\n", page, replacement_frame, kicked_page);
	
		// error handling
	if (replacement_frame < 0 || replacement_frame >= page_table_get_nframes(pt)) {
		fprintf(stderr, "Invalid frame index selected: %d\n", replacement_frame);
        exit(1);
	}

	if (kicked_page != -1) {
		int old_bits;
		int evicted_frame; 
		page_table_get_entry(pt, kicked_page, &evicted_frame, &old_bits);
		
		if (old_bits & BIT_DIRTY) {
			if (!disk || !physmem) {
				fprintf(stderr, "Disk or physical memory not initialized.\n");
                exit(1);
			}
			disk_write(disk, kicked_page, &physmem[evicted_frame * PAGE_SIZE]);
			writes2disk++;
		}
		page_table_set_entry(pt, kicked_page, 0, 0);
	}

	if (!disk || !physmem) {
		fprintf(stderr, "Disk or physical memory not initialized.\n");
        exit(1);
	}

	disk_read(disk, page, &physmem[replacement_frame * PAGE_SIZE]);
	reads2disk++;
	page_table_set_entry(pt, page, replacement_frame, BIT_PRESENT | BIT_WRITE);

	refSeq[replacement_frame] = 1;
	frame_table[replacement_frame] = page; 
	frequency[replacement_frame]++;
}

int main( int argc, char *argv[] )
{
	// performance tracking
	clock_t start_time = clock();

	// usage handling
	if(argc!=5) { 
		printf("usage: virtmem <npages> <nframes> <rand|clock|custom> <alpha|beta|gamma|delta>\n");
		return 1;
	}

	// extract values from command line 
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

	// performance tracking 
	clock_t end_time = clock();    // End timing
	double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

	printf("Page Faults: %d\n", faultCounter);
	printf("Disk Reads:  %d\n", reads2disk);
	printf("Disk Writes: %d\n", writes2disk);
	
	page_table_delete(pt);
	disk_close(disk);

	// performance tracking 
	printf("time to complete: %.6f s\n", elapsed_time);
	return 0;
}