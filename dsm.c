/* Authors: Purvi Patel (pp2j)
 *          Sabuj Pattanayek (sp2m)
 *          Shashi Singh Thakur (sst2m)
 * Purpose: Distributed Shared Memory malloc using MPI, see http://code.google.com/p/mtsudsm/
 * for details.
 * Professor: Dr. R. Butler
 * Course: CSCI6450, MTSU, S07
 */

#include "dsm.h"

void *mmptr[MAXDSMMALLOCS];
/* copy on read/write pointer, value == 1 read access, value == 2 write access */
int corwptr[MAXDSMMALLOCS];
int myRank, nextPtr = 0;
size_t pgsz;
pthread_t rootThread;

/* sp2m */
static void segv_handler(int signum, siginfo_t *si,void *ctx)
{
	int i;
	MPI_Status unused;

	/* Determine the mmptr that needs read/write access and also set it's corwptr to 1 or 2 */
	for (i = 0; i < nextPtr; i++) {
		if ((si->si_addr >= mmptr[i]) && (si->si_addr < (mmptr[i]+pgsz))) {
			printf("rank = %d SIGSEGV @ %p between %p and %p?\n", myRank, si->si_addr, mmptr[i], mmptr[i]+pgsz);
			/* Check to see if it already has read access, then it must want write access */
			if (corwptr[i] == 1) {
				if (mprotect(mmptr[i], pgsz, PROT_READ|PROT_WRITE) < 0) {
	    			perror("segv_handler, mprotect READ & WRITE");
					exit(-1);
				}
				corwptr[i] = 2;
				printf("rank = %d got PROT_READ|PROT_WRITE on page = %d\n", myRank, i);
			}
			else {
				/* else give it read/write for the receive */
				if (mprotect(mmptr[i], pgsz, PROT_READ|PROT_WRITE) < 0) {
	    			perror("segv_handler, mprotect READ & WRITE before receive from parent");
					exit(-1);
				}
				/* tell rank 0 to send the contents of the page pointed to by the ith mmptr */
				/* request for page, tag = 0, message = # of page */
				printf("rank = %d requesting page = %d\n", myRank, i);
				MPI_Send((void *) &i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
				/* get the page, tag = 2, message = the page */
				MPI_Recv(mmptr[i], pgsz, MPI_BYTE, 0, 2, MPI_COMM_WORLD, &unused); 
				printf("rank = %d received page = %d\n", myRank, i);
				/* give it read access only */
				if (mprotect(mmptr[i], pgsz, PROT_READ) < 0) {
	    			perror("segv_handler, mprotect READ");
					exit(-1);
				}
				corwptr[i] = 1;
			}
			break;
		}
	}
}
/* end sp2m */

/* sp2m */
void * handleWorkerRequests(void * unused) {
	int val, keepWorking = 1;
	MPI_Status status, unusedStatus;

	while(keepWorking) {
		MPI_Recv((void *) &val, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		/* Send the requesting source the page it wants */
		printf("Received MPI_TAG = %d\n", status.MPI_TAG);
		if (status.MPI_TAG == 0) {
			printf("parent sending page = %d to rank = %d\n", val, status.MPI_SOURCE);
			MPI_Send(mmptr[val], pgsz, MPI_BYTE, status.MPI_SOURCE, 2, MPI_COMM_WORLD);
		}
		/* Receive a page due to dsm_sync from a worker */
		else if (status.MPI_TAG == 1) {
			printf("parent receiving page = %d from rank = %d\n", val, status.MPI_SOURCE);
			MPI_Recv(mmptr[val], pgsz, MPI_BYTE, status.MPI_SOURCE, 2, MPI_COMM_WORLD, &unusedStatus);
		}
		/* This thread can stop working */
		else if (status.MPI_TAG == 3) {
			keepWorking = 0;
		}
		else {
			printf("Unknown MPI_TAG = %d\n received, quitting!\n", status.MPI_TAG);
			exit(-1);
		}
	}
	return unused;
}
/* end sp2m */

/* sst2m */
/* modified by sp2m */
int dsm_init(void)
{
	int rc, threadLevel;
	struct sigaction sa;

    rc = MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &threadLevel);
	if (threadLevel != MPI_THREAD_MULTIPLE) {
		printf("This program will not work without MPI_THREAD_MULTIPLE thread level support, quitting!\n");
		exit(rc);
	}
	/* get the page size */
	pgsz = getpagesize();
	/* setup the SIGSEGV handler */
	sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);

	/* setup a thread for handling sends and receives only in rank 0 */
	myRank = dsm_rank();
	if (myRank == 0) {
		pthread_create(&rootThread, NULL, handleWorkerRequests, (void *) NULL);
	}

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
	if (myRank != -1) {
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

	/* If nothing failed rank 0 gets read+write, all other ranks get PROT_NONE */
	if (myRank == 0) {
		printf("rank = %d mmap PROT_READ|PROT_WRITE mmptr[%d]\n", myRank, nextPtr);
		corwptr[nextPtr] = 2;
	}
	else {
		printf("rank = %d mmap PROT_NONE mmptr[%d]\n", myRank, nextPtr);
		corwptr[nextPtr] = 0;
	}
	/* increment the mmap ptr */
	nextPtr++;
	dsm_barrier();
    return mmptr[nextPtr-1];
}
/* end sp2m */

/* sp2m */
int dsm_nprocs(void) {
	int rc, numRanks;

	if ((rc = MPI_Comm_size(MPI_COMM_WORLD, &numRanks)) == MPI_SUCCESS) {
		return numRanks;
	}
	else {
		return rc;
	}
}
/* end sp2m */

/* sst2m */
/* modified by sp2m */
int dsm_barrier(void)
{
	printf("rank = %d calling barrier\n", myRank);
    return MPI_Barrier(MPI_COMM_WORLD);
}
/* end sst2m */

/* sp2m */
int dsm_sync(void)
{
	int i, rc = -1;

	for (i = 0; i < nextPtr; i++) {
		/* only synchronize mmptr's that have been written to and only if i'm not the parent rank */
		if (corwptr[i] == 2) {
			if (myRank == 0) {
				printf("rank = %d calling sync on page = %d\n", myRank, i);
				rc = MPI_SUCCESS;
			}
			else {
				/* tell rank 0 to prepare to receive the contents of the page pointed to by the ith mmptr */
				/* request to send a page, tag = 1, message = # of page */
				rc = MPI_Send((void *) &i, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
				/* send the page */
				rc = MPI_Send(mmptr[i], pgsz, MPI_BYTE, 0, 2, MPI_COMM_WORLD);
				/* set these pages to PROT_NONE */
				if (mprotect(mmptr[i], pgsz, PROT_NONE) < 0) {
				    perror("dsm_sync, mprotect NONE");
					return -1;
				}
				printf("rank = %d called sync and sent page = %d\n", myRank, i);
			}
		}
	}
	return rc;
}
/* end sp2m */

/* sst2m */
/* modified by sp2m */
int dsm_finalize(void)
{
	/* send a message to rank 0 to terminate the request handling thread */
	if (myRank == 0) {
		int i = 1;
		/* value = 1, tag = 3 */
		MPI_Send((void *) &i, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
		pthread_join(rootThread, NULL);
	}
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
