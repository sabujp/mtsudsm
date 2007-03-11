#include "dsm.h"

int main(int argc, char *argv[])
{
    int i, rc, nprocs, myrank;
    int *p[4];

    rc = dsm_init();
	myrank = dsm_rank();
	nprocs = dsm_nprocs();
    p[0] = dsm_malloc();
    p[myrank][100] = myrank;
    rc = dsm_sync();
    rc = dsm_barrier();
    if (myrank == 0)
    {
		for (i=0; i < nprocs; i++)
	    	printf("%d\n",p[i][100]);
    }

    rc = dsm_finalize();
	return 0;
}
