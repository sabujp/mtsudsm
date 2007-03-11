/* Authors: Purvi Patel (pp2j)
 *          Sabuj Pattanayek (sp2m)
 *          Shashi Singh Thakur (sst2m)
 * Purpose: Distributed Shared Memory malloc using MPI, see http://code.google.com/p/mtsudsm/
 * for details.
 * Professor: Dr. R. Butler
 * Course: CSCI6450, MTSU, S07
 */

#ifndef DSM_H
#define DSM_H

#include <stdio.h>
#include <pthreads.h>
#include <mpi.h>

__BEGIN_DECLS

/*int dsm_init(int *argc, char **argv);
 *sp2m 03/10/07 : see p1.txt, we can pass MPI_Init(null,null)
 */
int dsm_init(void);
/*int dsm_rank(MPI_Comm comm, int *rank);
 *sp2m 03/10/07 : once again see p1.txt, we will only use MPI_COMM_WORLD
 */
int dsm_rank(void);
int dsm_nprocs(void);
void * dsm_malloc(void);
int dsm_barrier(void);
int dsm_sync(void);
int dsm_finalize(void);
/*int dsm_mutex_lock(pthread_mutex_t *mutex);
 *sp2m 03/10/07 : we have to build our own structure of pthread mutexes, see p1.txt
 */
int dsm_mutex_lock(int);
/*int dsm_mutex_unlock(pthread_mutex_t *mutex);
 * same here!
 */
int dsm_mutex_unlock(int);
__END_DECLS   

#endif
