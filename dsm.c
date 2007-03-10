#include <stdio.h>
#include <stdlib.h>
#include "dsm.h"

int dsm_init(int *argc, ***argv)
{
	int rc;
    rc = MPI_Init(argc,argv);
    return rc;
}

int dsm_rank(MPI_Comm comm, int *rank)
{
	int rc;
    rc = MPI_Comm_rank(MPI_Comm_World , *rank);
    return rc;
}

void * dsm_malloc(void)
{
	
}
	

int dsm_barrier(MPI_Comm comm)
{
	int rc;
    rc = MPI_Barrier(MPI_Comm_world);
    return rc;
}

// int dsm_sync();
int dsm_finalize(void)
{
	return MPI_Finalize(void);

}

int dsm_mutex_lock(pthread_mutex_t *mutex)
{
	int rc ;
    rc = pthread_mutex_lock(pthread_mutex_t mutex);
    return rc;
}             

int dsm_mutex_unlock()
{
	int rc ;
    rc = pthread_mutex_unlock(pthread_mutex_t mutex);
    return rc;
}





