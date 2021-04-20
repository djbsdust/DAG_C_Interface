#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "aceMesh_runtime_c.h"

//#define N 128
int  T=10;

#ifndef TIME
#define IF_TIME(foo) foo;
#else
#define IF_TIME(foo)
#endif

#define min(a,b) ((a>b)?(b):(a))
#define max(a,b) ((a>b)?(a):(b))

double A[N][N];
double X[N][N];
double B[N][N];
double A1[N][N];
double X1[N][N];
double B1[N][N];

/////////////////////loop partitioning along ROW ////////////////////////
typedef struct{
  int ii;
  int block;
}TARGS_ROW;
void stencil_loop1(TARGS_ROW *args1){
  int ii;
  int j;
  int i=args1->ii;
  int block=args1->block;
  for(ii=i;ii<min(N,i+block);ii++)
  for (j = 1; j < N; j++) {
         X[ii][j] = X[ii][j] - X[ii][j-1] * A[ii][j] / B[ii][j-1];
         B[ii][j] = B[ii][j] - A[ii][j] * A[ii][j] / B[ii][j-1];
  }
}
void stencil_loop2(TARGS_ROW *args2){
  int i=args2->ii;
  int block=args2->block;
  int ii;
  for(ii=i;ii<min(N,i+block);ii++)
         X[ii][N-1] = X[ii][N-1] / B[ii][N-1];
}
void stencil_loop3(TARGS_ROW *args3){
  int i=args3->ii;
  int block=args3->block;
  int ii,j;
    for(ii=i;ii<min(N,i+block);ii++){
      for (j = 0; j < N-2; j++) {
         X[ii][N-j-2] = (X[ii][N-2-j] - X[ii][N-2-j-1] * A[ii][N-j-3]) / B[ii][N-3-j];
       }
     }
}
//////////////////////loop partitioning along COLUMN//////////////////////////
typedef struct{
  int jj;
  int block;
}TARGS_COL;
void stencil_loop4(TARGS_COL *args4){
  int j=args4->jj;
  int block=args4->block;
  int i,jj;
        for(jj=j;jj<min(N,j+block);jj++){
          for (i = 1; i < N; i++) {
                X[i][jj] = X[i][jj] - X[i-1][jj] * A[i][jj] / B[i-1][jj];
                B[i][jj] = B[i][jj] - A[i][jj] * A[i][jj] / B[i-1][jj];
      }      
     }
 }
void stencil_loop5(TARGS_COL *args5){
   int j=args5->jj;
   int block=args5->block;
   int jj=0;
    for (jj = j; jj <min(N,j+block); jj++) {
      X[N-1][jj] = X[N-1][jj] / B[N-1][jj]; 
     }     
     
 }
void stencil_loop6(TARGS_COL *args6){
  int j=args6->jj;
  int block=args6->block;
  int jj,i;
  for(jj=j;jj<min(N,j+block);jj++)
  for (i=0; i<N-2; i++) {
       X[N-2-i][jj] = (X[N-2-i][jj] - X[N-i-3][jj] * A[N-3-i][jj]) / B[N-2-i][jj];
  }
}

/////////////////////////////////////////////////////////////
void init_array(double A[][N],double X[][N],double B[][N]){
    int i, j;

    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            A[i][j] = (double)(i+1)*(j+1)/N;
        }
    }
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
           X[i][j] = (double)(i+1)*(j+1)/N;
        }
    }
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            B[i][j] = (double)((i+1)*(j+1))/N;
        }
    }
    
}

int check(){
  int i,j,t;
  init_array(A1,X1,B1);
    for (t = 0; t < T; t++) {
        for (i=0; i<N; i++) {
          for (j = 1; j < N; j++) {
                X1[i][j] = X1[i][j] - X1[i][j-1] * A1[i][j] / B1[i][j-1];
                B1[i][j] = B1[i][j] - A1[i][j] * A1[i][j] / B1[i][j-1];
            }
        }

        for (i=0; i<N; i++) {
                X1[i][N-1] = X1[i][N-1] / B1[i][N-1];
        }   

        for (i=0; i<N; i++) {
          for (j = 0; j < N-2; j++) {
                X1[i][N-j-2] = (X1[i][N-2-j] - X1[i][N-2-j-1] * A1[i][N-j-3]) / B1[i][N-3-j];
            }   
        }   

        for (i=1; i<N; i++) {
           for (j = 0; j < N; j++) {
                X1[i][j] = X1[i][j] - X1[i-1][j] * A1[i][j] / B1[i-1][j];
                B1[i][j] = B1[i][j] - A1[i][j] * A1[i][j] / B1[i-1][j];
            }   
        }   

        for (j=0; j<N; j++) {
                X1[N-1][j] = X1[N-1][j] / B1[N-1][j];
        }

        for (i = 0; i < N-2; i++) {
           for (j=0; j<N; j++) { //some person messed the subscripts of the next line previously
                X1[N-2-i][j] = (X1[N-2-i][j] - X1[N-i-3][j] * A1[N-3-i][j]) / B1[N-2-i][j];
            }
        }

    }
   
   for(i=0;i<N;i++)
   for(j=0;j<N;j++)
     if(fabs(X1[i][j]-X[i][j])>=1e-14){
        printf("i= %d,j= %d\n",i,j);
        printf("X1= %f ;X= %f\n",X1[i][j],X[i][j]);
        printf("------NO!!----\n");
        return 1;
     }//end if
   
   printf("*****check right!*****\n");
   return 0;
}

double rtclock(){
    struct timezone Tzp;
    struct timeval Tp;
    int stat;
    stat = gettimeofday (&Tp, &Tzp);
    if (stat != 0) printf("Error return from gettimeofday: %d",stat);
    return(Tp.tv_sec + Tp.tv_usec*1.0e-6);
}

int main(int argc,char **argv){
    
    int i,j,ii,t;
    int k=0;
    int blockx=128;
    int blocky=128;
    int threads=4;
    double t_start, t_end;
    TARGS_ROW args1, args2,args3;
    TARGS_COL args4, args5,args6;

	//loop, loop space,      partition which,  which neighbor
	//1:    [0:N-1, 1:N-1]     i			     none	
	//2:    [0:N-1, N-1]       i                 none
	//3:    [0:N-1, 0:N-2]     i                 none
	//                   ----barrier------
	//4:    [1:N-1, 0:N-1]     j                 none
    //5:    [N-1,   0:N-1]     j                 none
	//6:    [0:N-2, 0:N-1]     j                 none
    //                   ---barrier------

	//on data address mapping , from a data region to a single address

    //Apply global aligned loop tiling in your mind(NOT CORRECT, see following)
	//Phase-I  (loop 1,2,3):  tiles are [id][0]
	//Phase-II (loop 4,5,6):  tiles are [0][id]


    //  inter-loop-nest dependences:
    //      -----------------------  barrier        
    //              L1                             
    //            /    \                          
    //           /      \                         
    //          L2       \                      
	//                   L3                 
    //        -----------------------  barrier
    //                L4                        
    //              /     \                         
    //             /       \                          
    //            L5       L6                  
    //        -----------------------  barrier

    //after data address mapping
	//we will introduce edge <2-3>, <5-6>
	//but this will not hurt performance. I cannot see perf different between 3 after 2 and 2 after 3
    printf("input arg error, USAGE: ./a.out nter blkx blky num_threads\n");
	if(argc>1)
	  T=atoi(argv[1]);
    if(argc>2)
      blockx=atoi(argv[2]);
    if(argc>3)
      blocky=atoi(argv[3]);
    if(argc>4)
      threads=atoi(argv[4]);

    init_array(A,X,B);

     acemesh_runtime_init(threads); 
    IF_TIME(t_start = rtclock());
    
    for (t = 0; t < T; t++) {
	   //loop1:
	   //[0:N-1,1:N-1] 
	   //       X[i][j] = X[i][j] - X[i][j-1] * A[i][j] / B[i][j-1];
       //       B[i][j] = B[i][j] - A[i][j] * A[i][j] / B[i][j-1];
       acemesh_begin_split_task("loop1");
       for (i=0; i<N; i+=blockx) {
         args1.ii=i;
         args1.block=blockx;
         acemesh_push_wrlist(2, &X[i][0], NORMAL,&B[i][0],NORMAL);
         acemesh_push_rlist(1, &A[i][0], NORMAL);
         acemesh_task_generator((TASK_FUNCPTR)stencil_loop1,(void*)(&args1),sizeof(TARGS_ROW));
         acemesh_task_set_type(STENCIL_TASK);
       }
       acemesh_end_split_task();
       //loop2
	   //[0:N-1,N-1]
	   //   X[i][N-j-2] = (X[i][N-2-j] - X[i][N-2-j-1] * A[i][N-j-3]) / B[i][N-3-j];
	   acemesh_begin_split_task("loop2");
       for (i=0; i<N; i+=blockx) {
           args2.ii=i;
           args2.block=blockx;
           acemesh_push_wrlist(1, &X[i][0], NORMAL); 
           acemesh_push_rlist(2, &B[i][0], NORMAL, &A[i][0], NORMAL);
           acemesh_task_generator((TASK_FUNCPTR)stencil_loop2,(void*)(&args2),sizeof(TARGS_ROW));
           acemesh_task_set_type(STENCIL_TASK); 
        }
       acemesh_end_split_task();

	   //loop3:
	   //[0:N-1][0:N-2]
	   //   X[i][N-j-2] = (X[i][N-2-j] - X[i][N-2-j-1] * A[i][N-j-3]) / B[i][N-3-j];
       acemesh_begin_split_task("loop3");
       for (i=0; i<N; i+=blockx) {
		     int id=(i==0)?1:0; //for the first tile, we use address[0][1]
             args3.ii=i;
             args3.block=blockx;
             acemesh_push_wrlist(1, &X[i][0], NORMAL);
             acemesh_push_rlist(2, &A[i][0],NORMAL,&B[i][0], NORMAL);
             acemesh_task_generator((TASK_FUNCPTR)stencil_loop3,(void*)(&args3),sizeof(TARGS_ROW));
             acemesh_task_set_type(STENCIL_TASK);
       }
       acemesh_end_split_task();

       acemesh_spawn_and_wait(1);
       //loop4: 
	   //[1:N-1][0:N-1]
	   //  X[i][j] = X[i][j] - X[i-1][j] * A[i][j] / B[i-1][j];
	   //  B[i][j] = B[i][j] - A[i][j] * A[i][j] / B[i-1][j];
       acemesh_begin_split_task("loop4");
       for (j=0; j<N; j+=blocky) { 
          args4.jj=j;
          args4.block=blocky;
          acemesh_push_wrlist(2, &X[0][j], NORMAL,&B[0][j],NORMAL);         
          acemesh_push_rlist(1,&A[0][j],NORMAL);
          acemesh_task_generator((TASK_FUNCPTR)stencil_loop4,
                      (void*)(&args4), sizeof(TARGS_COL));
          acemesh_task_set_type(STENCIL_TASK);      
       }
       acemesh_end_split_task();
   
       //loop5: 
	   //[N-1][0:N-1]
	   //   X[N-1][j] = X[N-1][j] / B[N-1][j];
	   acemesh_begin_split_task("loop5");
       for(j=0; j<N; j+=blocky) {
          args5.jj=j;
          args5.block=blocky;
          acemesh_push_wrlist(1, &X[0][j], NORMAL);
          acemesh_push_rlist(1,&B[0][j],NORMAL);
          acemesh_task_generator((TASK_FUNCPTR)stencil_loop5,
                       (void*)(&args5), sizeof(TARGS_COL));
          acemesh_task_set_type(STENCIL_TASK);
       }
       acemesh_end_split_task();
      
       //loop6: 
	   //[0:N-2][0:N-1]
	   //   X[N-2-i][j] = (X[N-2-i][j] - X[N-i-3][j] * A[N-3-i][j]) / B[N-2-i][j];
	   acemesh_begin_split_task("loop6");
       for (j = 0; j <N; j+=blocky) {
            args6.jj=j;
            args6.block=blocky;
            acemesh_push_wrlist(1, &X[0][j], NORMAL);
            acemesh_push_rlist(2, &A[0][j],NORMAL,&B[0][j],NORMAL);
            acemesh_task_generator((TASK_FUNCPTR)stencil_loop6,
                       (void*)(&args6), sizeof(TARGS_COL));  
            acemesh_task_set_type(STENCIL_TASK);
       }
       acemesh_end_split_task();
       acemesh_spawn_and_wait(1);
    }
    /* pluto end */
    
	IF_TIME(t_end = rtclock());
    acemesh_runtime_shutdown(); 
    
#ifdef CHECK
   if(check()){
     printf("***error***\n");
     exit(1);
   }
#endif
    printf("threads= %d\n",threads);
    IF_TIME(printf("total_time %0.6lfs\n", t_end - t_start));
    
    return 0;
}
