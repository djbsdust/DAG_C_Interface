#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <mkl_blas.h>
//#include <f77blas.h>
#include "mkl_lapack.h"  //for spotrf_
#include <omp.h>
#include "tbb/tick_count.h"
using namespace tbb;
#include<iostream>
using namespace std;
//----------------------------------------------------------------------------------------------
#define real float
#define integer long
#define pi 3.1415926
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


//The code of this function has been directly extracted from the source code of the sgemm function of the f2c'd BLAS library (http://www.netlib.org/lapack/ here you'll find information and software related to LAPACK libraries and also BLAS)
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


//The code of this function has been directly extracted from the source code of the sgemm function of the f2c'd BLAS library (http:www.netlib.org/lapack/ here you'll find information and software related to LAPACK libraries and also BLAS)
 int inline ssyrk_2(char *uplo, char *trans, integer *n, integer *k,
         real *alpha, real *a, integer *lda, real *beta, real *c__, integer *
         ldc)
 {
//NOTE: this implementation seems wrong
//if you use this one, check failed.
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
    // sgemm_(
        // &NT, &TR,       /* TRANSA, TRANSB */
          // &NB, &NB, &NB,   /* M, N, K        */
          // &DMONE,          /* ALPHA          */
          // A, &NB,          /* A, LDA         */
          // B, &NB,          /* B, LDB         */
          // &DONE,           /* BETA           */
          // C, &NB);


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
    // strsm_(
        // &RI, &LO, &TR, &NU,  /* SIDE, UPLO, TRANSA, DIAG */
         // &NB, &NB,             /* M, N                     */
         // &DONE,                /* ALPHA                    */
         // T, &NB,               /* A, LDA                   */
         // B, &NB);

}

//#pragma css task input(A[NB][NB], NB,j) inout(C[NB][NB])
void ssyrk_tile( float *A, float *C, int NB, long j)
{
char LO='L', NT='N';
float DONE=1.0, DMONE=-1.0;
//you had better not use ssyrk_2, there seems ug in it.
//please use blas subroutine currently
//
     //ssyrk_2(&LO, &NT,          /* UPLO, TRANSA */
     //       (long*)&NB, (long*)&NB,           /* M, N         */
     //       &DMONE,             /* ALPHA        */
     //       A,(long*) &NB,             /* A, LDA       */
     //       &DONE,              /* BETA         */
     //       C, (long*)&NB);            /* C, LDC       */

 //using CBLAS
    ssyrk(
        &LO,&NT,
        &NB, &NB,
        &DMONE, A, &NB,
         &DONE, C, &NB);
    

}

//----------------------------------------------------------------------------------------------
void compute(struct timeval *start, struct timeval *stop, long NB, long DIM, float ***A)
{
  gettimeofday(start,NULL);
  long j,k,i; 
  
  for (j = 0; j < DIM; j++)
  {
    for (k= 0; k< j; k++)
    {
	   #pragma omp parallel for 
      for (i = j+1; i < DIM; i++) 
      {
        // A[i,j] = A[i,j] - A[i,k] * A[j,k]
        sgemm_tile( A[i][k], A[j][k], A[i][j], NB,i,j);
      }

    }
	//this is a reduction loop, there is inter-iteration data dependences
	//cannot use omp for.  by lchen
    for (i = 0; i < j; i++)
    {
      // A[j,j] = A[j,j] - A[j,i] * (A[j,i])
      ssyrk_tile( A[j][i], A[j][j], NB,j);
    }
    // Cholesky Factorization of A[j,j]
    spotrf_tile( A[j][j], NB,j);
	#pragma omp parallel for 
    for (i = j+1; i < DIM; i++)
    {
      // A[i,j] <- A[i,j] = X * (A[j,j])^t
      strsm_tile( A[j][j], A[i][j], NB,i,j);
    }
   
  }	
  //#pragma css finish
  gettimeofday(stop,NULL);
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
             cout<<"the value in Tiled A is : "<<A[j/NB][i/NB][(i%NB)*NB+j%NB]<<endl;
			 cout<<"but in original ALin, it is "<<Alin[i*N+j]<<endl;
             return 1;
          } 
    }
  }
  return 0;
}
static void  init(int argc, char **argv, int *NB_p, int *N_p, int *DIM_p, int *threads);
float ***A;
float * Alin; // private in init

int NB, N, DIM; // private in main
int main(int argc, char *argv[])
{
  // local vars
 long i;
 char LO='L';
 int  INFO;

 
 struct timeval start;
 struct timeval stop;
 unsigned long elapsed;
 int num_threads;
  float exctime;
  // application inicializations
  init(argc, argv, &NB, &N, &DIM, &num_threads);
  omp_set_num_threads(num_threads);
  compute(&start, &stop, NB, DIM, A);
  elapsed = 1000000*(stop.tv_sec - start.tv_sec);
  elapsed += stop.tv_usec - start.tv_usec;
  exctime=(float)elapsed/1000000;
  //compute with library
  spotrf_(&LO, &N, Alin, &N, &INFO);
  if(check(NB,DIM,NB*DIM,Alin,A)){
     cout<<"check error"<<endl;
	 exit(1);
  }
 cout<<"check is passed, correct!"<<endl;
 // time in usecs
 cout<< "exec time : "<<exctime<<endl;
 //performance in GFLOPS
 cout<<"using "<<num_threads<<" threads, totally "
     << (int)((0.33*N*N*N+0.5*N*N+0.17*N)/elapsed/1000)
     <<" GFLOPS"<<endl;

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
//
void randn(long n,float*ALin)
{
	srand((float)time(0));
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
int i;
static void init(int argc, char **argv, int *NB_p, int *N_p, int *DIM_p, int *num_threads)
{
  long ISEED[4] = {0,0,0,1};
  long  IONE=1;
  long  DIM;
  long  NB;
  long i;
 long j; 
 long k;
 int n_threads;
  if (argc==4)
  {
    NB=(long)atoi(argv[1]);
    DIM=(long)atoi(argv[2]);
	*num_threads=atoi(argv[3]);
  }
  else
  {
    cout<<"usage:"<<argv[0]<<" NB DIM num_threads\n"<<endl;
    exit(0);
  }

  // matrix init
  
  long N = NB*DIM;
  long NN = N * N;

  *NB_p=NB;
  *N_p = N;
  *DIM_p = DIM;
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

