#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>


//#define N 2048
//#define T 10

#ifndef TIME
#define IF_TIME(foo) foo;
#else
#define IF_TIME(foo)
#endif

double A[N][N];
double X[N][N];
double B[N][N];

void init_array(double A[][N],double X[][N],double B[][N])
{
    int i, j;

    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            A[i][j] = (double) (i+1)*(j+1)/N;
        }
    }
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
           X[i][j] = (double) (i+1)*(j+1)/N;
        }
    }
    for (i=0; i<N; i++) {
        for (j=0; j<N; j++) {
            B[i][j] = (double) (i+1)*(j+1)/N;
        }
    }
    
}


double rtclock()
{
    struct timezone Tzp;
    struct timeval Tp;
    int stat;
    stat = gettimeofday (&Tp, &Tzp);
    if (stat != 0) printf("Error return from gettimeofday: %d",stat);
    return(Tp.tv_sec + Tp.tv_usec*1.0e-6);
}

int main(int argc,char **argv)
{
    int i, j,t;


    double t_start, t_end;

    init_array(A,X,B);


    IF_TIME(t_start = rtclock());


    /* pluto start (T,N) */
    for (t = 0; t < T; t++) {
        //loop1:
        for (i=0; i<N; i++) {
            for (j = 1; j < N; j++) {
                X[i][j] = X[i][j] - X[i][j-1] * A[i][j] / B[i][j-1];

                B[i][j] = B[i][j] - A[i][j] * A[i][j] / B[i][j-1];
            }
        }
		//loop2:
        for (i=0; i<N; i++) {
                X[i][N-1] = X[i][N-1] / B[i][N-1];
        }
		//loop3:
        for (i=0; i<N; i++) {
            for (j = 0; j <= N-2; j++) { //bug! beyond array A's bounds
                X[i][N-j-2] = (X[i][N-2-j] - X[i][N-2-j-1] * A[i][N-j-3]) / B[i][N-3-j];
            }
        }
        //loop4: 
        for (i=1; i<N; i++) {
            for (j = 0; j < N; j++) {
                X[i][j] = X[i][j] - X[i-1][j] * A[i][j] / B[i-1][j];
                B[i][j] = B[i][j] - A[i][j] * A[i][j] / B[i-1][j];
            }
        }
        //loop5:
        for (j=0; j<N; j++) {
                X[N-1][j] = X[N-1][j] / B[N-1][j];
        }
        //loop6:
        for (i = 0; i <= N-2; i++) {  //bug! beyond array A's bounds
            for (j=0; j<N; j++) {
                X[N-2-i][j] = (X[N-2-i][j] - X[N-i-3][j] * A[N-3-i][j]) / B[N-2-i][j];
            }
        }

    }
    /* pluto end */
    IF_TIME(t_end = rtclock());
    IF_TIME(printf("%0.6lfs\n", t_end - t_start));


    return 0;
}
