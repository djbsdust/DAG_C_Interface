#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <mkl_blas.h>
#include "mkl_lapack.h"  //for spotrf_
//#include <cblas.h>
//#include <f77blas.h>
//#include <omp.h>
#include <iostream>
using namespace std;
#include "tbb/task.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "aceMesh_runtime_c.h"
using namespace tbb;
//----------------------------------------------------------------------------------------------
#define real float
#define integer long
#define pi 3.1415926
float elapsed;
int NB;

// The code of this function has been directly extracted from the source code of the sgemm function of the f2c'd BLAS library (http://www.netlib.org/lapack/ here you'll find information and software related to LAPACK libraries and also BLAS)
int inline sgemm_2(char *transa, char *transb, integer *m, integer *
        n, integer *k, real *alpha, real *a, integer *lda, real *b, integer *
        ldb, real *beta, real *c__, integer *ldc)
{
    integer a_dim1, a_offset, b_dim1, b_offset, c_dim1, c_offset, i__1, i__2,i__3;
    real temp;
    integer i__, j, l;
#define a_ref(a_1,a_2) a[(a_2)*a_dim1 + a_1]
#define b_ref(a_1,a_2) b[(a_2)*b_dim1 + a_1]
#define c___ref(a_1,a_2) c__[(a_2)*c_dim1 + a_1]
   a_dim1 = *lda;
    a_offset = 1 + a_dim1 * 1;
    a -= a_offset;
    b_dim1 = *ldb;
    b_offset = 1 + b_dim1 * 1;
    b -= b_offset;
    c_dim1 = *ldc;
    c_offset = 1 + c_dim1 * 1;
    c__ -= c_offset;
    i__1 = *n;

    for (j = 1; j <= i__1; ++j) {
        i__2 = *k;
        for (l = 1; l <= i__2; ++l) {
            temp = *alpha * b_ref(j, l);
            i__3 = *m;
            for (i__ = 1; i__ <= i__3; ++i__)
                c___ref(i__, j) = c___ref(i__, j) + temp * a_ref(i__, l);
        }
    }

    return 0;
}

#undef c___ref
#undef b_ref
#undef a_ref


// The code of this function has been directly extracted from the source code of the sgemm function 
//  in the f2c'd BLAS library, please refer to  
//  (http://www.netlib.org/lapack/ to find more information 
//  and software related to LAPACK libraries and also BLAS)
int inline strsm_2(char *side, char *uplo, char *transa, char *diag,
        integer *m, integer *n, real *alpha, real *a, integer *lda, real *b,
        integer *ldb)
{
    integer a_dim1, a_offset, b_dim1, b_offset, i__1, i__2, i__3;
    real temp;
    integer i__, j, k;

#define a_ref(a_1,a_2) a[(a_2)*a_dim1 + a_1]
#define b_ref(a_1,a_2) b[(a_2)*b_dim1 + a_1]
    a_dim1 = *lda;
    a_offset = 1 + a_dim1 * 1;
    a -= a_offset;
    b_dim1 = *ldb;
    b_offset = 1 + b_dim1 * 1;
    b -= b_offset;
    i__1 = *n;

    for (k = 1; k <= i__1; ++k)
    {
        temp = 1.f / a_ref(k, k);
        i__2 = *m;

        for (i__ = 1; i__ <= i__2; ++i__)
            b_ref(i__, k) = temp * b_ref(i__, k);
        i__2 = *n;

        for (j = k + 1; j <= i__2; ++j) 
        {
            temp = a_ref(j, k);
            i__3 = *m;

            for (i__ = 1; i__ <= i__3; ++i__) 
                b_ref(i__, j) = b_ref(i__, j) - temp * b_ref(i__, k);

        }
    }
    return 0;
}
#undef b_ref
#undef a_ref


// The code of this function has been directly extracted from the source code of the sgemm function of the f2c'd BLAS library (http://www.netlib.org/lapack/ here you'll find information and software related to LAPACK libraries and also BLAS)
int inline ssyrk_2(char *uplo, char *trans, integer *n, integer *k,
        real *alpha, real *a, integer *lda, real *beta, real *c__, integer *
        ldc)
{
    integer a_dim1, a_offset, c_dim1, c_offset, i__1, i__2, i__3;
    real temp;
    integer i__, j, l;
#define a_ref(a_1,a_2) a[(a_2)*a_dim1 + a_1]
#define c___ref(a_1,a_2) c__[(a_2)*c_dim1 + a_1]
    a_dim1 = *lda;
    a_offset = 1 + a_dim1 * 1;
    a -= a_offset;
    c_dim1 = *ldc;
    c_offset = 1 + c_dim1 * 1;
    c__ -= c_offset;
    i__1 = *n;

    for (j = 1; j <= i__1; ++j)
    {
        i__2 = *k;

        for (l = 1; l <= i__2; ++l)
        {
            temp = *alpha * a_ref(j, l);
            i__3 = *n;

            for (i__ = j; i__ <= i__3; ++i__) 
                c___ref(i__, j) = c___ref(i__, j) + temp * a_ref(i__, l);

        }
    }

    return 0;
}
#undef c___ref
#undef a_ref

#undef real 
#undef integer

// ---------------------------------------------------------------------------------------------------

//#pragma css task input(NB,j) inout(A[NB][NB]) highpriority 
//
void spotrf_tile(float *A,int NB,int j)
{
   int INFO;
   char LO='L';

   spotf2_(&LO,
          &NB,
          A, &NB,
          &INFO);

}

//#pragma css task input(A[NB][NB], B[NB][NB], NB,i,j) inout(C[NB][NB])
void sgemm_tile(float  *A, float *B, float *C,  long NB, long i, long j)
{
char TR='T', NT='N';
float DONE=1.0, DMONE=-1.0;

   sgemm_2(&NT, &TR,       /* TRANSA, TRANSB */
          &NB, &NB, &NB,   /* M, N, K        */
          &DMONE,          /* ALPHA          */
          A, &NB,          /* A, LDA         */
          B, &NB,          /* B, LDB         */
          &DONE,           /* BETA           */
          C, &NB);         /* C, LDC         */

 //using CBLAS
    // cblas_sgemm(
        // CblasColMajor,
         // CblasNoTrans, CblasTrans,
         // NB, NB, NB,
         // -1.0, A, NB,
               // B, NB,
          // 1.0, C, NB);


}

//#pragma css task input(T[NB][NB], NB,i,j) inout(B[NB][NB])
void strsm_tile(float *T, float *B, long NB, long i, long j){
char LO='L', TR='T', NU='N', RI='R';
float DONE=1.0;

  strsm_2(&RI, &LO, &TR, &NU,  /* SIDE, UPLO, TRANSA, DIAG */
         &NB, &NB,             /* M, N                     */
         &DONE,                /* ALPHA                    */
         T, &NB,               /* A, LDA                   */
         B, &NB);              /* B, LDB                   */

 //using CBLAS
    // cblas_strsm(
        // CblasColMajor,
        // CblasRight, CblasLower, CblasTrans, CblasNonUnit,
        // NB, NB,
        // 1.0, T, NB,
             // B, NB);

}

//#pragma css task input(A[NB][NB], NB,j) inout(C[NB][NB])
void ssyrk_tile( float *A, float *C, int NB, long j)
{
char LO='L', NT='N';
float DONE=1.0, DMONE=-1.0;

    // ssyrk_2(&LO, &NT,          /* UPLO, TRANSA */
           // &NB, &NB,           /* M, N         */
           // &DMONE,             /* ALPHA        */
           // A, &NB,             /* A, LDA       */
           // &DONE,              /* BETA         */
           // C, &NB);            /* C, LDC       */

 //using CBLAS
    ssyrk(
        &LO, &NT,
        &NB, &NB,
        &DMONE, A, &NB,
         &DONE, C, &NB);


}
//spotrf---------------------------------------------------------------------------------------------
typedef struct{
	float *A;
	long i;
}TARGS1; // for spotrf_tile(A,NB,i);
void __spotrf_tile(TARGS1 *args1){
   spotrf_tile(args1->A,NB,args1->i);
}
typedef struct{
	float *A, *B;
	long i;
}TARGS2; //ssyrk_tile(A,B,NB,i);
void __ssyrk_tile(TARGS2 *args2){
   ssyrk_tile(args2->A,args2->B,NB,args2->i);
}

typedef struct{
	float *A, *B, *C;
	long i,j;
}TARGS3; //sgemm_tile(A,B,C,NB,i,j);
void __sgemm_tile(TARGS3 *args3){
   sgemm_tile(args3->A,args3->B,args3->C,NB,args3->i, args3->j);
}

typedef struct{
	float *A, *B;
	long i,j;
}TARGS4; //strsm_tile(A,B,NB,i,j);
void __strsm_tile(TARGS4 *args4){
   strsm_tile(args4->A,args4->B,NB,args4->i, args4->j);
}

void compute(struct timeval *start, struct timeval *stop, long DIM, float ***A)
{
  tbb::tick_count mainStartTime = tbb::tick_count::now();
  long j,k,i; 
  for (j = 0; j < DIM; j++)
  {
    for (k= 0; k< j; k++)
    {
		acemesh_begin_split_task("sgemm_tile");
	  // #pragma omp parallel for
      for (i = j+1; i < DIM; i++) 
      {
        // A[i,j] = A[i,j] - A[i,k] * A[j,k]
        //sgemm_tile( A[i][k], A[j][k], A[i][j], NB,i,j);
	     TARGS3 args3;	
         args3.A=A[i][k], args3.B= A[j][k], args3.C= A[i][j],
         args3.i=i, args3.j=j;
         acemesh_push_rlist(2, &A[i][k], NORMAL,&A[j][k],NORMAL);
         acemesh_push_wrlist(1, &A[i][j], NORMAL);
         acemesh_task_generator((TASK_FUNCPTR)__sgemm_tile,(void*)(&args3),sizeof(TARGS3));
      }
     acemesh_end_split_task();
    }
    for (i = 0; i < j; i++)
    {
      //ssyrk_tile( A[j][i], A[j][j], NB,j);
	  acemesh_begin_split_task("ssyrk_tile");
      // A[j,j] = A[j,j] - A[j,i] * (A[j,i])
	  TARGS2 args2;	
      args2.A=A[j][i], args2.B= A[j][j], args2.i=j;
      acemesh_push_rlist(1, &A[j][i], NORMAL);
      acemesh_push_wrlist(1, &A[j][j], NORMAL);
      acemesh_task_generator((TASK_FUNCPTR)__ssyrk_tile,(void*)(&args2),sizeof(TARGS2));
	  acemesh_end_split_task();
    }
    // Cholesky Factorization of A[j,j]
    //spotrf_tile( A[j][j], NB,j);
	acemesh_begin_split_task("spotrf_tile");
	TARGS1 args1;	
    args1.A=A[j][j], args1.i=j;
    acemesh_push_wrlist(1, &A[j][j], NORMAL);
    acemesh_task_generator((TASK_FUNCPTR)__spotrf_tile,(void*)(&args1),sizeof(TARGS1));
	acemesh_end_split_task();
	acemesh_begin_split_task("strsm_tile");
    for (i = j+1; i < DIM; i++)
    {
      // A[i,j] <- A[i,j] = X * (A[j,j])^t
      // strsm_tile( A[j][j], A[i][j], NB,i,j);
	  TARGS4 args4;	
      args4.A=A[j][j], args4.B= A[i][j], args4.i=i, args4.j=j;
      acemesh_push_rlist(1, &A[j][j], NORMAL);
      acemesh_push_wrlist(1, &A[i][j], NORMAL);
      acemesh_task_generator((TASK_FUNCPTR)__strsm_tile,(void*)(&args4),sizeof(TARGS4));
    }
     acemesh_end_split_task();
  }	
  acemesh_spawn_and_wait(1);  
  tbb::tick_count mainEndTime = tbb::tick_count::now();
  elapsed =(mainEndTime-mainStartTime).seconds();
}

//--------------------------------------------------------------------------------

static int check(long NB,long DIM, long N, float *Alin, float ***A)
{
  long i,j,k;
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
		
      if(fabs(A[j/NB][i/NB][(i%NB)*NB+j%NB]-Alin[i*N+j])> 1e-4)
		  {
             cout<<"check inconsistent!"<<endl;
             cout<<"in tile ["<<j/NB<<"]["<<i/NB<<"], element "<<(i%NB)*NB+j%NB<<" is wrong."<<endl;
             cout<<"the value in tiled A is : "<<A[j/NB][i/NB][(i%NB)*NB+j%NB]<<endl;
			 cout<<"but in original ALin, it is "<<Alin[i*N+j]<<endl;
             return 1;
          } 
    }
  }
  return 0;
}
static void  init(int argc, char **argv, int *NB_p, int *N_p, int *DIM_p,int *thread_num);
float ***A;
float * Alin; // private in init

int N, DIM; // private in main
int main(int argc, char *argv[])
{
  // local vars
 long i;
 char LO='L';
 int  INFO;
 struct timeval start;
 struct timeval stop;
 int thread_num;
 // application inicializations
 init(argc, argv, &NB, &N, &DIM,&thread_num);
 // compute with CellSs
 acemesh_runtime_init(thread_num);
 //tbb::tick_count mainStartTime = tbb::tick_count::now();
 //funciton version
 compute(&start, &stop, DIM, A);
 //tbb::tick_count mainEndTime = tbb::tick_count::now();
 acemesh_runtime_shutdown();
 //compute with library
  spotrf_(&LO, &N, Alin, &N, &INFO);
  if(check(NB,DIM,NB*DIM,Alin,A))
    exit(1);
   cout<<"correct!"<<endl;
   // time in seconds
   cout<<"total_time : "<<elapsed<<",";
   // performance in GFLOPS
   cout<<"Using "<<thread_num<<" threads, totally "
	   <<(int)((0.33*N*N*N+0.5*N*N+0.17*N)/elapsed/1000000000)<<" GFLOPS"<<endl;

	return 0;
}
void convert_to_blocks(long NB,long DIM, long N, float *Alin , float ***A)
{
  long i,j,k;
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      A[j/NB][i/NB][(i%NB)*NB+j%NB]= Alin[i*N+j];
    }
  }
}


//void slarnv_(long *idist, long *iseed, long *n, float *x);

void randn(long n,float*ALin)
{
	srand((int)time(0));
	int i,j;
	float temp;
	for(i=0;i<n*n;i++)
	{
		Alin[i]=rand()%2;
	}
	for(i=0;i<n;i++){
     for(j=0;j<n;j++)
     {
         temp=Alin[i*n+j]+Alin[j*n+j];
         Alin[i*n+j]=temp;
         Alin[j*n+i]=temp;
     }
	}
     for(i=0;i<n;i++){
        Alin[i*n+i]=0;
	 }
}
static void init(int argc, char **argv, int *NB_p, int *N_p, int *DIM_p,int *thread_num)
{
  long ISEED[4] = {0,0,0,1};
  long  IONE=1;
  long  DIM;
  long  NB;
  long i;
 long j; 
 long k;
 int th;
  if (argc==4)
  {
    NB=(long)atoi(argv[1]);
    DIM=(long)atoi(argv[2]);
	th=(int)atoi(argv[3]);
  }
  else
  {
    cout<<"usage: "<< argv[0]<<"NB DIM thread_num"<<endl;
    exit(0);
  }

  // matrix init
  
  long N = NB*DIM;
  long NN = N * N;
  
  *NB_p=NB;
  *N_p = N;
  *DIM_p = DIM;
  *thread_num=th;
  // linear matrix
   Alin = (float *) malloc(NN * sizeof(float));

  // fill the matrix with random values
  randn(N,Alin);
// make it positive definite
  for(i=0; i<N; i++)
  {
    Alin[i*N + i] += N;
  }
  // blocked matrix
  A = (float ***) malloc(DIM*sizeof(float **));
  for (i = 0; i < DIM; i++)
  {
	  A[i] = (float **) malloc(DIM*sizeof(float*));
      for(j=0;j<DIM;j++)
		  A[i][j]=(float *)malloc(NB*NB*sizeof(float));
  }

  convert_to_blocks(NB, DIM, N, Alin, A);
   
}

