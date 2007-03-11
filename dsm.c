/* Authors: Purvi Patel (pp2j)
 *          Sabuj Pattanayek (sp2m)
 *          Shashi Singh Thakur (sst2m)
 * Purpose: Distributed Shared Memory malloc using MPI, see http://code.google.com/p/mtsudsm/
 * for details.
 * Professor: Dr. R. Butler
 * Course: CSCI6450, MTSU, S07
 */

#include <stdio.h>
#include <stdlib.h>
#include "dsm.h"
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

void *mmptr[MAXDSMMALLOCS];
/* copy on read/write pointer, value == 1 read access, value == 2 write access */
int corwptr[MAXDSMMALLOCS];
int pgsz;
int nextPtr = 0;

/* sp2m */
static void segv_handler(int signum, siginfo_t *si,void *ctx)
{
	int i;
	MPI_Status unused;

	/* Determine the mmptr that needs read/write access and also set it's corwptr to 1 or 2 */
	for (i = 0; i < nextPtr; i++) {
		if ((si->si_addr >= mmptr[i]) && (si->si_addr < (mmptr+pgsz)[i])) {
			/* Check to see if it already has read access, then it must want write access */
			if (corwptr[i] == 1) {
				if (mprotect(mmptr[i], pgsz, PROT_READ|PROT_WRITE) < 0) {
	    			perror("segv_handler, mprotect READ & WRITE");
					exit(-1);
				}
				corwptr[i] = 2;
			}
			else {
				/* else give it read access only */
				if (mprotect(mmptr[i], pgsz, PROT_READ) < 0) {
	    			perror("segv_handler, mprotect READ");
					exit(-1);
				}
				/* tell rank 0 to send the contents of the page pointed to by the ith mmptr */
				/* request for page, tag = 0, message = # of page */
				MPI_Send((void *) &i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
				/* get the page, tag = 2, message = the page */
				MPI_Recv(mmptr[i], pgsz, MPI_BYTE, 0, 2, MPI_COMM_WORLD, &unused); 
				corwptr[i] = 1;
			}
			break;
		}
	}
}
/* end sp2m */

/* sst2m */
/* modified by sp2m */
int dsm_init(void)
{
	int rc;
	struct sigaction sa;

    rc = MPI_Init(NULL, NULL);
	/* get the page size */
	pgsz = getpagesize();
	/* setup the SIGSEGV handler */
	sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);

	/* setup a thread for handling sends and receives only in rank 0 */

    return rc;
}
/* end sst2m */

/* sst2m */
/* modified by sp2m */
int dsm_rank(void)
{
	int rc, rank;
	if ((rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank)) != MPI_SUCCESS) {
		return -1;
	}
	else {
		return rank;
	}
}
/* end sst2m */

/* sp2m */
void * dsm_malloc(void)
{
	int myRank;
	
	if ((myRank = dsm_rank()) != -1) {
		/* Rank 0 has immediate read/write access */
		if (myRank == 0) {
		    mmptr[nextPtr] = mmap(0, pgsz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		}
		/* All other ranks cannot read/write until SIGSEGV is handled and rank 0 sends the page
		 * back to the rank requesting read/write */
		else {
		    mmptr[nextPtr] = mmap(0, pgsz, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		}
    	if (mmptr[nextPtr] == MAP_FAILED) {
			/* failure, don't increment the mmap ptr */
	    	perror("dsm_malloc, mmap");
			dsm_barrier();
			return mmptr[nextPtr];
		}
	}
	else {
		/* failure, don't increment the mmap ptr */
		printf("dsm_malloc, dsm_rank\n");
		dsm_barrier();
		return MAP_FAILED;
	}
	
	/* page does not have read or write access */
	corwptr[nextPtr] = 0;
	/* increment the mmap ptr */
	nextPtr++;
	dsm_barrier();
    return mmptr[nextPtr];
}
/* end sp2m */

/* sst2m */
/* modified by sp2m */
int dsm_barrier(void)
{
    return MPI_Barrier(MPI_COMM_WORLD);
}
/* end sst2m */

/* sp2m */
int dsm_sync(void)
{
	int i, rc = -1;

	for (i = 0; i < nextPtr; i++) {
		/* only synchronize mmptr's that have been written to */
		if (corwptr[i] == 2) {
			/* set these pages to PROT_NONE */
			if (mprotect(mmptr[i], pgsz, PROT_NONE) < 0) {
			    perror("dsm_sync, mprotect NONE");
				return -1;
			}
			/* tell rank 0 to prepare to receive the contents of the page pointed to by the ith mmptr */
			/* request to send a page, tag = 1, message = # of page */
			MPI_Send((void *) &i, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
			/* send the page */
			rc = MPI_Send(mmptr[i], pgsz, MPI_BYTE, 0, 2, MPI_COMM_WORLD);
		}
	}
	return rc;
}
/* end sp2m */

/* sst2m */
int dsm_finalize(void)
{
	return MPI_Finalize();
}
/* end sst2m */

/* sst2m */
int dsm_mutex_lock(int mutex)
{
	/*
	int rc;
    rc = pthread_mutex_lock(pthread_mutex_t mutex);
    return rc;
	*/
	return 0;
}          
/* end sst2m */

/* sst2m */
int dsm_mutex_unlock(int mutex)
{
	/*
	int rc ;
    rc = pthread_mutex_unlock(pthread_mutex_t mutex);
    return rc;
	*/
	return 0;
}
/* end sst2m */
