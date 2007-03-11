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
#include <unistd>

/* sp2m */
static void segv_handler(int signum, siginfo_t *si,void *ctx)
{
    printf("HANDLING SEGV\n");
    printf("probarea=%p\n", si->si_addr);
    if (mprotect(mmptr, pgsz, PROT_READ|PROT_WRITE) < 0)
    perror("mprotect in handler");
}
/* end sp2m */

/* sst2m */
int dsm_init(int *argc, ***argv)
{
	int rc;
    rc = MPI_Init(argc,argv);
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
	void *mmptr;
	int pgsz, myRank;
    struct sigaction sa;
	
    pgsz = getpagesize();
    sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
	
	if ((myRank = dsm_rank()) != -1) {
		/* Rank 0 has immediate read/write access */
		if (myRank == 0) {
		    mmptr = mmap(0, pgsz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		}
		/* All other ranks cannot read/write until SIGSEGV is handled and rank 0 sends the page
		 * back to the rank requesting read/write */
		else {
		    mmptr = mmap(0, pgsz, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		}
    	if (mmptr == MAP_FAILED) {
	    	perror("dsm_malloc, mmap");
			return mmptr;
		}
	}
	else {
		printf("dsm_malloc, dsm_rank\n");
		return MAP_FAILED;
	}
			
    /*printf("mmptr=%p\n",mmptr);
    ((char*)mmptr)[99] = 'X';
    printf("x=%c\n", ((char*)mmptr)[99] );
	
    if (mprotect(mmptr,pgsz,PROT_NONE) < 0)
    perror("mprotect");
    ((char*)mmptr)[99] = 'Y';
    printf("y=%c\n", ((char*)mmptr)[99] );*/
	
    return mmptr;
}
/* end sp2m */

/* sst2m */
int dsm_barrier(MPI_Comm comm)
{
	int rc;
    rc = MPI_Barrier(MPI_Comm_world);
    return rc;
}
/* end sst2m */

/* sp2m */
int dsm_sync(void)
{

}
/* end sp2m */

/* sst2m */
int dsm_finalize(void)
{
	return MPI_Finalize(void);

}
/* end sst2m */

/* sst2m */
int dsm_mutex_lock(pthread_mutex_t *mutex)
{
	int rc ;
    rc = pthread_mutex_lock(pthread_mutex_t mutex);
    return rc;
}          
/* end sst2m */

/* sst2m */
int dsm_mutex_unlock()
{
	int rc ;
    rc = pthread_mutex_unlock(pthread_mutex_t mutex);
    return rc;
}
/* end sst2m */
