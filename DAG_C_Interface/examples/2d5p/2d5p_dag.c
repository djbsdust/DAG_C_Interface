//you should define SIZEX/SIZEY/SIZEZ/BLKX/BLKY/ITER
//actually 2d-partitioning in this program 

//#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <sys/time.h>
#include "aceMesh_runtime_c.h"

double A[2][SIZEX][SIZEY]; //__attribute__ ((aligned(64)));
//for test
double test[2][SIZEX][SIZEY];
int BLKX, BLKY;

//arg
#define alpha_d 0.0876
#define beta_d  0.0765
//kernel 
#define Do2d5pkernel(dest,src,i,j) {\
   A[dest][i][j] = alpha_d * (A[(src)][(i)][(j)]) + \
                        beta_d * (A[src][i-1][j] + A[src][i+1][j] +\
                        A[src][i][j-1] + A[src][i][j+1] +\
                        A[src][i][j] + A[src][i][j]);}
//
#define TestDo2d5pkernel(itr,i,j) {\
   test[itr%2][i][j] = alpha_d * test[(itr+1)%2][i][j] + \
                        beta_d * (test[(itr+1)%2][i-1][j] + test[(itr+1)%2][i+1][j] +\
                        test[(itr+1)%2][i][j-1] + test[(itr+1)%2][i][j+1] +\
                        test[(itr+1)%2][i][j] + test[(itr+1)%2][i][j] ); } 

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
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(src_addr - BLKX * SIZEY  * 8);
    if(starti + BLKX < SIZEX)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(src_addr + BLKX * SIZEY  * 8);
    if(startj - BLKY >= 0)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(src_addr - BLKY * 8);
    if(startj + BLKY < SIZEY)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(src_addr + BLKY * 8);
}
void stencil_core(targs* args)
{
    int starti=args->starti;
    int endi=args->endi;
    int startj=args->startj;
    int endj=args->endj;
    int iter=args->iter;
    int i,j;

    int dest = iter  % 2 ;
    int src = 1 - dest;
    for(i = starti; i < endi; i++)
    for(j = startj; j < endj; j++)
          Do2d5pkernel(dest,src,i,j);
}


int Do2d5p(const int iter, const int blocsizex, const int blocsizey)
{
    targs each;
	int itr,i,j;
    const int ntaski = SIZEX / blocsizex;
    const int ntaskj = SIZEY / blocsizey;

    for(itr = 0; itr < iter; ++itr)
    {
        acemesh_begin_split_task("2d5p iter");
        for(i = 1; i < SIZEX -1; i += blocsizex)
        {
            int endi = (i + blocsizex > SIZEX -1 ) ? SIZEX - 1 : i + blocsizex;
            int indexi = (i-1) / blocsizex;
            for(j = 1; j < SIZEY -1; j += blocsizey)
            {
                int indexj = (j-1) / blocsizey;
                int endj = (j + blocsizey > SIZEY -1 ) ? SIZEY - 1 : j + blocsizey;
                each.starti=i;
                each.endi=endi;
                each.startj=j;
                each.endj=endj;
                each.iter=itr;
                each.src_addr=&A[(itr+1)%2][i][j];
                acemesh_push_wlist(1, &A[itr%2][i][j],NORMAL);
                acemesh_push_rlist(1, &A[(itr+1)%2][i][j], NORMAL);
                acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_core,
                      (void*)(&each), sizeof(targs), 
                      NULL, (NEIGHBOR_FUNCPTR)my_neighbors, &each);
                acemesh_task_set_type(STENCIL_TASK);
            }
        }
        acemesh_end_split_task();
    }
    acemesh_spawn_and_wait(1);
    return 0;
}



void Myinit()
{
    int i,j,k;
    for( i = 0; i < SIZEX; i++)
    for( j = 0; j < SIZEY; j++)
        A[1][i][j] = (double)(i*2.5 + j*3.3 ) /3.0; 
}
int check(const int iter)
{
   int i,j,k;
   int itr;
   int ii,jj,kk;
    
   printf("begin check\n");
   for(i = 0; i < SIZEX; i++)
   for( j = 0; j < SIZEY; j++)
      test[1][i][j] = (double)(i*2.5 + j*3.3 ) /3.0; 
       
   for(itr = 0; itr < iter; ++itr){
      for(i = 1; i < SIZEX - 1; i++)
      for( j = 1; j < SIZEY - 1; j++)
         TestDo2d5pkernel(itr,i,j);
   }
       
   for(ii = 1; ii < SIZEX - 1; ii++)
   for(jj = 1; jj < SIZEY - 1; jj++)
         if( fabs(test[(iter+1)%2][ii][jj]  - A[(iter+1)%2][ii][jj]) > 1e-14 ){
             printf("i: %d   j:  %d \n",ii,jj);    
             printf("test: %lf",test[(iter+1)%2][ii][jj]);
             printf("but A: %lf",A[(iter+1)%2][ii][jj]);
             printf("they are different!\n");               
             return -1;
         }
   printf("correct\n");
   return 0;
}



int main(int argc, char** argv)
{
    int num_threads;
    int i,j,k;
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
    Myinit();
    acemesh_runtime_init(num_threads);
    
	gettimeofday(&start,NULL);
    Do2d5p(ITER, BLKX, BLKY);
    gettimeofday(&end,NULL);   
    
	acemesh_runtime_shutdown();  
#ifdef CHECK
    total_time = 0.0;
    total_time+=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
    if(!check(ITER))
        printf("total time: %lf\n",total_time);       
    else 
    {
        printf("check_error\n");
        exit(1);
    }
#endif
    return 0;
}
