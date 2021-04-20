//you should define SIZEX/SIZEY/SIZEZ/BLKX/BLKY/ITER
//actually 2d-partitioning in this program

//#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <sys/time.h>
#include "aceMesh_runtime_c.h"

double A[2][SIZEX][SIZEY][SIZEZ]; //__attribute__ ((aligned(64)));
//for test
double test[2][SIZEX][SIZEY][SIZEZ];
int BLKX, BLKY;

//arg
#define alpha_d 0.0876
#define beta_d  0.0765
//kernel
#define Do3d7pkernel(dest,src,i,j,k) {\
A[dest][i][j][k] = alpha_d * (A[(src)][(i)][(j)][(k)]) + \
                        beta_d * (A[src][i-1][j][k] + A[src][i+1][j][k] +\
                        A[src][i][j-1][k] + A[src][i][j+1][k] +\
                        A[src][i][j][k-1] + A[src][i][j][k+1]);}
//
#define TestDo3d7pkernel(itr,i,j,k) {\
test[itr%2][i][j][k] = alpha_d * test[(itr+1)%2][i][j][k] + \
                        beta_d * (test[(itr+1)%2][i-1][j][k] + test[(itr+1)%2][i+1][j][k] +\
                        test[(itr+1)%2][i][j-1][k] + test[(itr+1)%2][i][j+1][k] +\
                        test[(itr+1)%2][i][j][k-1] + test[(itr+1)%2][i][j][k+1] ); }

typedef struct{
  int starti, endi;
  int startj, endj;
  int iter;
  void* src_addr;
}targs;

void my_neighbors(struct ptr_array *neighbor_addrs, targs* args)
{
    int starti=args->starti;
    int startj=args->startj;
    void* src_addr=args->src_addr;

    neighbor_addrs->len=0;
    if(starti - BLKX>=0)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(src_addr - BLKX * SIZEY * SIZEZ * 8);
    if(starti + BLKX < SIZEX)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(src_addr + BLKX * SIZEY * SIZEZ * 8);
    if(startj - BLKY >= 0)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(src_addr - BLKY * SIZEZ * 8);
    if(startj + BLKY < SIZEY)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(src_addr + BLKY * SIZEZ* 8);
}

void stencil_init(targs* args)
{
    int starti=args->starti;
    int endi=args->endi;
    int startj=args->startj;
    int endj=args->endj;
    int i,j,k;

    for(i = starti; i < endi; i++)
      for(j = startj; j < endj; j++)
        for(k = 0; k < SIZEZ; k++)
        {
            A[0][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
            A[1][i][j][k] = A[0][i][j][k];
        }
}

void stencil_core(targs* args)
{
    int starti=args->starti;
    int endi=args->endi;
    int startj=args->startj;
    int endj=args->endj;
    int iter=args->iter;
    int i,j,k;

    int dest = iter  % 2 ;
    int src = 1 - dest;

#ifdef MERGE_INIT
//not correct
//when computing task accesses neighbor_data, neighbor_task has not been executed and the data has not been initialized.
//only for some test
    if(0)
    //if(iter==0)
	{
      int temp_starti = (starti==1)?0:starti;
      int temp_endi=(endi>=SIZEX-1)?SIZEX:endi;
      int temp_startj = (startj==1)?0:startj;
      int temp_endj=(endj>=SIZEY-1)?SIZEY:endj;
      //printf("%d,%d,%d,%d\n",starti,startj,endi,endj);
      //printf("%d,%d,%d,%d\n",temp_starti,temp_startj,temp_endi,temp_endj);
      for(i=temp_starti; i < temp_endi; i++)
      {
         for(j=temp_startj; j < temp_endj; j++)
           for(k = 0; k < SIZEZ; k++)
           {
             A[0][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
             A[1][i][j][k] = A[0][i][j][k];
           }
	  }
    }
#endif

    for(i = starti; i < endi; i++)
      for(j = startj; j < endj; j++)
        //for(k = 1; k < args->k_N - 1; k++)
        for(k = 1; k < SIZEZ - 1; k++)
		{
          Do3d7pkernel(dest,src,i,j,k);
		  }
}

int Do3d7p(const int iter, const int blocsizex, const int blocsizey, const int num_threads)
{
    targs each;
    int itr,i,j;
//    const int ntaski = SIZEX / blocsizex;
//    const int ntaskj = SIZEY / blocsizey;

#ifdef GRAPH_REUSE
//#define GRAPH_DIST 25
    int outer_itr;
    for(outer_itr = 0; outer_itr < iter/GRAPH_DIST; outer_itr++)
    {
    int ret = acemesh_dag_start(1);
    if (ret)
        goto exec_3d7p;
    for(itr = 0; itr < GRAPH_DIST; itr++)
#else
    for(itr = 0; itr < iter; ++itr)
#endif
    {
#ifdef GP
        if(itr==2) acemesh_end_win(num_threads/16);
#endif
        acemesh_begin_split_task("3d7p iter");
#ifdef INIT_BLOCKED
        acemesh_arraytile(&A[itr%2][0][0][0],"A",SIZEX,blocsizex,2);
        acemesh_arraytile(&A[1-itr%2][0][0][0],"A",SIZEX,blocsizex,1);
#endif
        for(i = 1; i < SIZEX -1; i += blocsizex)
        {

            int endi = (i + blocsizex > SIZEX -1 ) ? SIZEX - 1 : i + blocsizex;
#ifdef INIT_BLOCKED
            int indexi = (i) / blocsizex;
#endif
            for(j = 1; j < SIZEY -1; j += blocsizey)
            {
//                int indexj = (j-1) / blocsizey;
                int endj = (j + blocsizey > SIZEY -1 ) ? SIZEY - 1 : j + blocsizey;
                each.starti=i;
                each.endi=endi;
                each.startj=j;
                each.endj=endj;
                each.iter=itr;
#ifdef OFFSET
                each.src_addr=&A[(itr+1)%2][i][j+LOACL_OFFSET][0];
                acemesh_push_wlist(1, &A[itr%2][i][j+LOACL_OFFSET][0],NORMAL);
                acemesh_push_rlist(1, &A[(itr+1)%2][i][j+LOACL_OFFSET][0], NORMAL);

#else
                each.src_addr=&A[(itr+1)%2][i][j][0];
                acemesh_push_wlist(1, &A[itr%2][i][j][0],NORMAL);
                acemesh_push_rlist(1, &A[(itr+1)%2][i][j][0], NORMAL);
#endif
                acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_core,
                      (void*)(&each), sizeof(targs),
                      NULL, (NEIGHBOR_FUNCPTR)my_neighbors, &each);
#ifdef INIT_BLOCKED
                acemesh_affinity_from_arraytile(indexi,ceil((double)(SIZEX-1-1)/blocsizex),num_threads);
//                acemesh_task_set_affinity(-1);
#endif
                acemesh_task_set_type(STENCIL_TASK);
            }
        }
        acemesh_end_split_task();
    }
#ifdef GRAPH_REUSE
    exec_3d7p:
#endif
    acemesh_spawn_and_wait(1);
    //acemesh_wait_for_all_task();
#ifdef GRAPH_REUSE
    }
#endif
    return 0;
}



void Myinit()
{
    int i,j,k;
    for(i = 0; i < SIZEX; i++)
    for(j = 0; j < SIZEY; j++)
    for(k = 0; k < SIZEZ; k++)
    {
      double temp = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
      A[0][i][j][k] = temp;
      A[1][i][j][k] = temp;
      test[0][i][j][k] = temp;
      test[1][i][j][k] = temp;
    }
}
#ifdef INIT_CYCLIC
void Myinit_cyclic_dag(const int blocsizex, const int blocsizey)
{
    int i,j,k;
    for(i = 0; i < SIZEX; i++)
    for(j = 0; j < SIZEY; j++)
    for(k = 0; k < SIZEZ; k++)
    {
      test[1][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
      test[0][i][j][k] = test[1][i][j][k];
    }
#ifndef MERGE_INIT
    targs each;
    acemesh_begin_split_task("3d7p init_cyclic");
    for(i = 1; i < SIZEX -1; i += blocsizex)
    {
		int starti = (i==1) ? 0 : i;
        //int endi = (i + blocsizex > SIZEX -1 ) ? SIZEX - 1 : i + blocsizex;
        int endi = (i + blocsizex >= SIZEX -1 ) ? SIZEX : i + blocsizex;
        for(j = 1; j < SIZEY -1; j += blocsizey)
        {
			int startj = (j==1) ? 0 : j;
            //int endj = (j + blocsizey > SIZEY -1 ) ? SIZEY - 1 : j + blocsizey;
            int endj = (j + blocsizey >= SIZEY -1 ) ? SIZEY : j + blocsizey;

			  each.starti=starti;
			  each.endi=endi;
			  each.startj=startj;
			  each.endj=endj;
#ifdef OFFSET
            acemesh_push_wlist(1, &A[0][i][j+LOACL_OFFSET][0],NORMAL);
            acemesh_push_rlist(1, &A[1][i][j+LOACL_OFFSET][0], NORMAL);
#else
            acemesh_push_wlist(1, &A[0][i][j][0],NORMAL);
            acemesh_push_wlist(1, &A[1][i][j][0], NORMAL);
#endif
            acemesh_task_generator((TASK_FUNCPTR)stencil_init,
                  (void*)(&each), sizeof(targs));
            acemesh_task_set_type(STENCIL_TASK);
        }
    }
    acemesh_end_split_task();
    //acemesh_spawn_and_wait(1);
/*    //init boundary data of array[0]
    for(int i = 0; i < SIZEX; i+=SIZEX-1)
    for(int j = 0; j < SIZEY; j++)//=SIZEY-1)
    for(int k = 0; k < SIZEZ; k++)//=SIZEZ-1)
    {
      A[0][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
      A[1][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
    }
    for(int i = 1; i < SIZEX-1; i++)//=SIZEX-1)
    for(int j = 0; j < SIZEY; j+=SIZEY-1)
    for(int k = 0; k < SIZEZ; k++)//=SIZEZ-1)
    {
      A[0][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
      A[1][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
    }
    for(int i = 1; i < SIZEX-1; i++)//=SIZEX-1)
    for(int j = 1; j < SIZEY-1; j++)//+=SIZEY-1)
    for(int k = 0; k < SIZEZ; k+=SIZEZ-1)
    {
      A[0][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
      A[1][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
    }*/
#endif
}
#endif
#ifdef INIT_BLOCKED
void Myinit_blocked_dag(int blocsizex,int blocsizey,int num_threads)
{
    int i,j,k;
    for(i = 0; i < SIZEX; i++)
    for(j = 0; j < SIZEY; j++)
    for(k = 0; k < SIZEZ; k++)
    {
      test[1][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
      test[0][i][j][k]=test[1][i][j][k];
    }
    targs each;
    acemesh_arraytile(&A[1][0][0][0],"A",SIZEX,blocsizex,2);
    acemesh_arraytile(&A[0][0][0][0],"A",SIZEX,blocsizex,2);
    acemesh_begin_split_task("3d7p init_blocked");
    for(i = 1; i < SIZEX-1; i += blocsizex){
		int starti = (i==1) ? 0 : i;
        //int endi = (i + blocsizex > SIZEX-1  ) ? SIZEX-1  : i + blocsizex;
        int endi = (i + blocsizex >= SIZEX -1 ) ? SIZEX : i + blocsizex;
        int indexi = i / blocsizex;

        for(j = 1; j < SIZEY-1 ; j += blocsizey){
            acemesh_push_wlist(1, &A[0][i][j][0],NORMAL);
            acemesh_push_wlist(1, &A[1][i][j][0],NORMAL);
        }
        each.starti=starti;
        each.endi=endi;
        each.startj=0;
        each.endj=SIZEY;
        acemesh_task_generator((TASK_FUNCPTR)stencil_init,
              (void*)(&each), sizeof(targs));
        acemesh_task_set_type(STENCIL_TASK);
        acemesh_affinity_from_arraytile(indexi,ceil((double)(SIZEX-1-1)/blocsizex),num_threads);
    }
    acemesh_end_split_task();
    acemesh_spawn_and_wait(1);
    //acemesh_end_win();

}
#endif
int check(const int iter)
{
   int i,j,k;
   int itr;
   int ii,jj,kk;
   printf("begin check\n");

    for(itr = 0; itr < iter; ++itr){
     for(i = 1; i < SIZEX - 1; i++)
      for( j = 1; j < SIZEY - 1; j++)
       for( k = 1; k < SIZEZ - 1; k++)
	   {
         TestDo3d7pkernel(itr,i,j,k);
		 }
      }

    for(ii = 1; ii < SIZEX - 1; ii++)
      for(jj = 1; jj < SIZEY - 1; jj++)
       for(kk = 1; kk < SIZEZ - 1; kk++){
          if( fabs(test[(iter+1)%2][ii][jj][kk]  - A[(iter+1)%2][ii][jj][kk]) > 1e-10 ){
             printf("i: %d   j:  %d k: %d\n",ii,jj,kk);
             printf("test: %lf",test[(iter+1)%2][ii][jj][kk]);
             printf("but A: %lf",A[(iter+1)%2][ii][jj][kk]);
             printf("they are different!\n");
             return -1;
         }
      }
  printf("correct\n");
  return 0;
}



int main(int argc, char** argv)
{
    int num_threads;
    //int i,j,k;
    struct timeval start;
    struct timeval end;
    double total_time;

    if(argc > 5)
    {
        printf("input arg error, USAGE: ./a.out nter blkx blky num_threads\n");
        return 0;
    }
    num_threads = 1;
    BLKX = 16;
    BLKY = 16;
    int ITER = 1;
    if(argc > 1)
        ITER = atoi(argv[1]);
    if(argc > 2)
        BLKX = atoi(argv[2]);
    if(argc > 3)
        BLKY = atoi(argv[3]);
    if(argc > 4)
        num_threads = atoi(argv[4]);
    acemesh_runtime_init(num_threads);
#ifdef GP
    acemesh_start_win();
#endif

#ifdef INIT_SERIAL
    Myinit();
#elif INIT_CYCLIC
    Myinit_cyclic_dag(BLKX,BLKY);
#elif INIT_BLOCKED
    Myinit_blocked_dag(BLKX,BLKY,num_threads);
#endif

    gettimeofday(&start,NULL);
//#ifdef INIT_BLOCKED
    Do3d7p(ITER, BLKX, BLKY,num_threads);
//#else
//    Do3d7p(ITER, BLKX, BLKY);
//#endif
    gettimeofday(&end,NULL);

    acemesh_runtime_shutdown();
    total_time = 0.0;
    total_time+=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
    printf("total time: %lf\n",total_time);
#ifdef CHECK
    if(check(ITER))
    {
        printf("check_error\n");
        exit(1);
    }
#endif
    return 0;
}
