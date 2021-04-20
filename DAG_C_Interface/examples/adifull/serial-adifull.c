#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>

//#define N 128
int T=40;

#ifndef TIME
#define IF_TIME(foo) foo;
#else
#define IF_TIME(foo)
#endif

#define min(a,b) ((a>b)?(b):(a))

double A[N][N];
double X[N][N];
double B[N][N];
double A1[N][N];
double X1[N][N];
double B1[N][N];

void init_array(double A[][N],double X[][N],double B[][N]){
    int i, j;
    //we cannot let B[i][j]=0.0
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
            B[i][j] = (double)(i+1)*(j+1)/N;
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
           for (j=0; j<N; j++) {//some person messed the subscripts of the next line previously 
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
     }
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
    
    int i,j,ii,jj,t;
    int blockx=128;
    
    int blocky=128;
    double t_start, t_end;
	if(argc>1)
	  T=atoi(argv[1]);
    if(argc>2)
      blockx=atoi(argv[2]);
    if(argc>3)
      blocky=atoi(argv[3]);

    init_array(A,X,B);

    IF_TIME(t_start = rtclock());
    
    /* pluto start (T,N) */
    for (t = 0; t < T; t++) {
       for (i=0; i<N; i+=blockx) 
       for (ii=i;ii<min(N,i+blockx);ii++)
       for (j = 1; j < N; j++) {
                X[ii][j] = X[ii][j] - X[ii][j-1] * A[ii][j] / B[ii][j-1];
                B[ii][j] = B[ii][j] - A[ii][j] * A[ii][j] / B[ii][j-1];
       }
       for (i=0; i<N; i+=blockx) 
       for(ii=i;ii<min(N,i+blockx);ii++)
            X[ii][N-1] = X[ii][N-1] / B[ii][N-1];
       for (i=0; i<N; i+=blockx) 
       for (ii=i;ii<min(N,i+blockx);ii++)
       for (j = 0; j < N-2; j++) //upper bound bug fix 
            X[ii][N-j-2] = (X[ii][N-2-j] - X[ii][N-2-j-1] * A[ii][N-j-3]) / B[ii][N-3-j];
         
       for (jj = 0; jj < N; jj+=blocky) 
       for(j=jj;j<min(N,jj+blocky);j++)
       for (i=1; i<N; i++) {
                X[i][j] = X[i][j] - X[i-1][j] * A[i][j] / B[i-1][j];
                B[i][j] = B[i][j] - A[i][j] * A[i][j] / B[i-1][j];
            
       }
       for (jj=0; jj<N; jj+=blocky) 
       for(j=jj;j<min(N,jj+blocky);j++) 
                X[N-1][j] = X[N-1][j] / B[N-1][j];
          
       for (jj=0; jj<N; jj+=blocky) 
       for(j=jj;j<min(N,jj+blocky);j++)
       for (i = 0; i < N-2; i++)//upper bound bug fix 
                X[N-2-i][j] = (X[N-2-i][j] - X[N-i-3][j] * A[N-3-i][j]) / B[N-2-i][j];
    
    } /* pluto end */
    
    IF_TIME(t_end = rtclock());
#ifdef CHECK
   if(check()){
    printf("exec time : ***error***\n");
    exit(1);
   }
#endif
    IF_TIME(printf("exec time : %0.6lfs\n", t_end - t_start));
    
    return 0;
}
