//---------------------------------------------------------------------------------
//6 loops are partitioned along two dimensions, there are pipeline loops, incurs lots of stealing
//auto-affinity may not be good.
//for 2-depth loop nests, if put pipeloop inside then there is only one kind of affinity.
//                        if put parallel loop inside, we can have more affinity choices, but may still be limited for many-core platform
//                        and there will be conflicts between row sweeping and column sweeping 
//use MACRO  -DMAN affinity, you can trigger on the manual affinity setting.
//BUT cyclic mapping over linearlized id is not a good idea, pipeline advances slower...
//You should find a better manual affinity strategy, when one dimension cannot provide enough affinity_id.
//----------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "aceMesh_runtime_c.h"

//#define N 128
int  T=10;
int block1, block2;

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

/////////////////////loop partitioning along two dimensions , one pipeline loop ////////////////////////
typedef struct{
  int ii, jj;
}TARGS;
void stencil_loop1(TARGS *args){
  int ii,fromj;
  int j,fromi;
  fromi=args->ii;
  fromj=args->jj;
  for(ii=fromi;ii<min(N,fromi+block1);ii++)
  for (j = max(1,fromj); j <min( N,fromj+block2); j++) {
         X[ii][j] = X[ii][j] - X[ii][j-1] * A[ii][j] / B[ii][j-1];
         B[ii][j] = B[ii][j] - A[ii][j] * A[ii][j] / B[ii][j-1];
  }
}

void neighbor_loop1(struct ptr_array *neighbor_addrs,TARGS *args)
{
 //B[i,j-1], X[i,j-1]
 int ii=args->ii;
 int jj=args->jj;
 neighbor_addrs->len=0;
 if(jj<block2)  return;
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&B[ii][jj-block2]);
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&X[ii][jj-block2]);
}

void stencil_loop2(TARGS *args){
  int fromi=args->ii;
  int ii;
  for(ii=fromi;ii<min(N,fromi+block1);ii++)
         X[ii][N-1] = X[ii][N-1] / B[ii][N-1];
}

void stencil_loop3(TARGS *args){
  int i=args->ii;
  int jj=args->jj;
  int ii,j;
    for(ii=i;ii<min(N,i+block1);ii++){
      for (j =max(jj, 0); j < min(N-2,jj+block2); j++) {
         X[ii][N-j-2] = (X[ii][N-2-j] - X[ii][N-2-j-1] * A[ii][N-j-3]) / B[ii][N-3-j];
       }
     }
}


void neighbor_loop3(struct ptr_array *neighbor_addrs,TARGS *args)
{
 //X[i,N-2-j-1],A[i][N-2-j-1]; B[i][N-2-j-1]
 int ii=args->ii;
 int jj=N-2-(args->jj+block2-1);

 neighbor_addrs->len=0;
 if(jj<=0)   return;
 jj=jj-block2;
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&X[ii][jj]);
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&A[ii][jj]);
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&B[ii][jj]);
 }

void stencil_loop4(TARGS *args){
  int fromj=args->jj;
  int fromi=args->ii;
  int i,jj;

  for(jj=fromj;jj<min(N,fromj+block2);jj++)
  for (i = max(1,fromi); i < min(N,fromi+block1); i++) {
     X[i][jj] = X[i][jj] - X[i-1][jj] * A[i][jj] / B[i-1][jj];
     B[i][jj] = B[i][jj] - A[i][jj] * A[i][jj] / B[i-1][jj];
  }      
 }

void neighbor_loop4(struct ptr_array *neighbor_addrs,TARGS *args)
{
 //X[i-1,j],B[i-1][j]
 int ii=args->ii;
 int jj=args->jj;

 neighbor_addrs->len=0;
 if(ii<block1)   return;
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&X[ii-block1][jj]);
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&B[ii-block1][jj]);
 }


void stencil_loop5(TARGS *args){
   int fromj=args->jj;
   int jj=0;
   for (jj = fromj; jj <min(N,fromj+block2); jj++) {
      X[N-1][jj] = X[N-1][jj] / B[N-1][jj]; 
   }     
     
 }
void stencil_loop6(TARGS *args){
  int fromj=args->jj;
  int fromi=args->ii;
  int jj,i;
  for(jj=fromj;jj<min(N,fromj+block2);jj++)
  for (i =max(fromi, 0); i < min(N-2,fromi+block1); i++)
  {
       X[N-2-i][jj] = (X[N-2-i][jj] - X[N-i-3][jj] * A[N-3-i][jj]) / B[N-2-i][jj];
  }
}

void neighbor_loop6(struct ptr_array *neighbor_addrs,TARGS *args)
{
 //X1[N-2-i-1][j] * A1[N-2-i-1][j]
 int jj=args->jj;
 int ii=N-2-(args->ii+block1-1);

 neighbor_addrs->len=0;
 if(ii<=0)   return;
 ii=ii-block1;
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&X[ii][jj]);
 neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(&A[ii][jj]);
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
    TARGS args;

    printf("input arg error, USAGE: ./a.out nter blkx blky num_threads\n");
    if(argc>1)
      T=atoi(argv[1]);
    if(argc>2)
      blockx=atoi(argv[2]);
    if(argc>3)
      blocky=atoi(argv[3]);
    if(argc>4)
      threads=atoi(argv[4]);
  
   block1=blockx; block2=blocky;
   init_array(A,X,B);

   acemesh_runtime_init(threads); 
   IF_TIME(t_start = rtclock());
#ifdef MAN_AFFINITY
   int size1=ceil((float)N/blockx);
   int size2=ceil((float)N/blocky);
   int fid;
#endif   
 
   for (t = 0; t < T; t++) 
   {
     //loop1:
     //       X[i][j] = X[i][j] - X[i][j-1] * A[i][j] / B[i][j-1];
     //       B[i][j] = B[i][j] - A[i][j] * A[i][j] / B[i][j-1];
     for (j=0; j<N; j+=blocky)
     {
       acemesh_begin_split_task("loop1");
       for (i=0; i<N; i+=blockx) 
       {
         args.ii=i;
         args.jj=j;
         acemesh_push_wrlist(2, &X[i][j], NORMAL,&B[i][j],NORMAL);
         acemesh_push_rlist(1, &A[i][j], NORMAL);
         acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_loop1,(void*)(&args),sizeof(TARGS)
                      ,NULL, (NEIGHBOR_FUNCPTR)neighbor_loop1, &args);
         acemesh_task_set_type(STENCIL_TASK);
#ifdef MAN_AFFINITY
         fid=(i/block1*size2+j/block2)%threads;
         acemesh_task_set_affinity(fid);
#endif
       }
       acemesh_end_split_task();
     }//pipeline loop
           
     //loop2
     //   X1[i][N-1] = X1[i][N-1] / B1[i][N-1]
     //   addr:[i] [N-1-(N-1)%blocky]
     acemesh_begin_split_task("loop2");
     for (i=0; i<N; i+=blockx)
     {
           args.ii=i;
           int jj=N-1-(N-1)%blocky;
           acemesh_push_wrlist(1, &X[i][jj], NORMAL); 
           acemesh_push_rlist(2, &B[i][jj], NORMAL, &A[i][jj], NORMAL);
           acemesh_task_generator((TASK_FUNCPTR)stencil_loop2,(void*)(&args),sizeof(TARGS));
           acemesh_task_set_type(STENCIL_TASK); 
#ifdef MAN_AFFINITY
           fid=(i/block1*size2+j/block2)%threads;
           acemesh_task_set_affinity(fid);
#endif
     }
     acemesh_end_split_task();

     //loop3: visiting array's j-dimension reversely
     //  j:0~N-3,
     //  X[i][N-j-2] = (X[i][N-2-j] - X[i][N-2-j-1] * A[i][N-j-3]) / B[i][N-3-j];

     //for loop j, for lhs
     //first data tile's addr:  N-2- (N-2)%blocky
     //let first loop tile full length
     int from= (N-2)%blocky+1-blocky;
     for (j=from; j<N-2; j+=blocky)
     {
            acemesh_begin_split_task("loop3");
            for (i=0; i<N; i+=blockx) 
            {
             args.ii=i;
             args.jj=j;
             int addrj=N-2-(j+blocky-1);
             acemesh_push_wrlist(1, &X[i][addrj], NORMAL);
             acemesh_push_rlist(2, &A[i][addrj],NORMAL,&B[i][addrj], NORMAL);
             acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_loop3
                         ,(void*)(&args),sizeof(TARGS)
                         ,NULL, (NEIGHBOR_FUNCPTR)neighbor_loop3, &args);
             acemesh_task_set_type(STENCIL_TASK);
#ifdef MAN_AFFINITY
             fid=(i/block1*size2+addrj/block2)%threads;
             acemesh_task_set_affinity(fid);
#endif
             }//parallel loop
             acemesh_end_split_task();
     }//pipeline
       
     //loop4: 
     //[1:N-1][0:N-1]
     //  X[i][j] = X[i][j] - X[i-1][j] * A[i][j] / B[i-1][j];
     //  B[i][j] = B[i][j] - A[i][j] * A[i][j] / B[i-1][j];
       
     for (i=0; i<N; i+=block1) 
     {
       acemesh_begin_split_task("loop4");
       for (j=0; j<N; j+=block2) { 
          args.jj=j;
          args.ii=i;
          acemesh_push_wrlist(2, &X[i][j], NORMAL,&B[i][j],NORMAL);         
          acemesh_push_rlist(1,&A[i][j],NORMAL);
          acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_loop4,
                      (void*)(&args), sizeof(TARGS)
                       ,NULL, (NEIGHBOR_FUNCPTR)neighbor_loop4, &args );
          acemesh_task_set_type(STENCIL_TASK);      
#ifdef MAN_AFFINITY
          fid=(i/block1*size2+j/block2)%threads;
          acemesh_task_set_affinity(fid);
#endif
       }//parallel loop
       acemesh_end_split_task();
     }//pipeline loop

     //loop5: 
     //[N-1][0:N-1]
     //   X[N-1][j] = X[N-1][j] / B[N-1][j];
     acemesh_begin_split_task("loop5");
     for(j=0; j<N; j+=blocky) {
          args.jj=j;
          int ii=N-1-(N-1)%blockx;
          acemesh_push_wrlist(1, &X[ii][j], NORMAL);
          acemesh_push_rlist(1,&B[ii][j],NORMAL);
          acemesh_task_generator((TASK_FUNCPTR)stencil_loop5,
                      (void*)(&args), sizeof(TARGS));
          acemesh_task_set_type(STENCIL_TASK);
#ifdef MAN_AFFINITY
          fid=(ii/block1*size2+j/block2)%threads;
          acemesh_task_set_affinity(fid);
#endif
     }
     acemesh_end_split_task();
      
     //loop6: 
     //[0:N-2][0:N-1]
     //   X[N-2-i][j] = (X[N-2-i][j] - X[N-i-3][j] * A[N-3-i][j]) / B[N-2-i][j];
     from=  (N-2)%block1+1-block1;
     for (i=from; i<N-2; i+=block1){ 
       acemesh_begin_split_task("loop6");
       for (j = 0; j <N; j+=blocky) {
            args.jj=j;
            args.ii=i;
            int addri=N-2-(i+blockx-1);
            acemesh_push_wrlist(1, &X[addri][j], NORMAL);
            acemesh_push_rlist(2, &A[addri][j],NORMAL,&B[addri][j],NORMAL);
            acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_loop6,
                       (void*)(&args), sizeof(TARGS)
                        ,NULL, (NEIGHBOR_FUNCPTR)neighbor_loop6, &args);
            acemesh_task_set_type(STENCIL_TASK);
#ifdef MAN_AFFINITY
            fid=(addri/block1*size2+j/block2)%threads;
            acemesh_task_set_affinity(fid);
#endif
       }//parallel loop
       acemesh_end_split_task();
     }//pipeline loop
           
   }//time iteration
   acemesh_spawn_and_wait(1);
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
