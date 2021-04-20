#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cassert>
#include<sys/time.h>
#include <omp.h>
using namespace std;

//you had better not define SIZE directly here, but use env variables when make
//#define SIZE 256
double A[2][SIZEX][SIZEY][SIZEZ]; //__attribute__ ((aligned(64)));
//for comparison with A array
double test[2][SIZEX][SIZEY][SIZEZ];


//args
#define alpha_d 0.0876
#define beta_d  0.0765

//real kernel 
#define Do3d7pkernel(dest,src,i,j,k) {\
   A[dest][i][j][k] = alpha_d * (A[(src)][(i)][(j)][(k)]) + \
                        beta_d * (A[src][i-1][j][k] + A[src][i+1][j][k] +\
                        A[src][i][j-1][k] + A[src][i][j+1][k] +\
                        A[src][i][j][k-1] + A[src][i][j][k+1]);}
//for comparison
#define TestDo3d7pkernel(itr,i,j,k) {\
   test[itr%2][i][j][k] = alpha_d * test[(itr+1)%2][i][j][k] + \
                        beta_d * (test[(itr+1)%2][i-1][j][k] + test[(itr+1)%2][i+1][j][k] +\
                        test[(itr+1)%2][i][j-1][k] + test[(itr+1)%2][i][j+1][k] +\
                        test[(itr+1)%2][i][j][k-1] + test[(itr+1)%2][i][j][k+1] ); } 




int Do3d7p(const int ITER, const int blocsizex, const int blocsizey, int thread_num)
{

    for(int itr = 0; itr < ITER; ++itr)
    {
#pragma omp parallel for collapse(2)
        for(int i = 1; i < SIZEX -1; i += blocsizex)
        for(int j = 1; j < SIZEY -1; j += blocsizey)
        {
            int endi = (i + blocsizex > SIZEX -1 ) ? SIZEX - 1 : i + blocsizex;
            int indexi = (i-1) / blocsizex;
            int indexj = (j-1) / blocsizey;
            int endj = (j + blocsizey > SIZEY -1 ) ? SIZEY - 1 : j + blocsizey;
			bool dest = itr  % 2 ;
            bool src = 1 - dest;
            for(int ii = i; ii < endi; ii++)
              for(int jj = j; jj < endj; jj++)
                for(int k = 1; k <  SIZEZ - 1; k++) 
                    Do3d7pkernel(dest,src,ii,jj,k);
            }//j loop
    }//itr loop
    return 0;
}


void init()
{
    for(int i = 0; i < SIZEX; i++)
    for(int j = 0; j < SIZEY; j++)
    for(int k = 0; k < SIZEZ; k++)
    {
      A[1][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
      test[1][i][j][k]=A[1][i][j][k];
    }
	//init boundary data of array[0]
    for(int i = 0; i < SIZEX; i+=SIZEX-1)
    for(int j = 0; j < SIZEY; j+=SIZEY-1)
    for(int k = 0; k < SIZEZ; k+=SIZEZ-1)
    {
      A[0][i][j][k] = (double)(i*2.5 + j*3.3 + k*0.5 ) /3.0;
      test[0][i][j][k]=A[0][i][j][k];
    }
	
}
int check(const int ITER)
{
    cout<<"begin check\n";
    for(int itr = 0; itr < ITER; ++itr)
    {
        for(int i = 1; i < SIZEX - 1; i++)
            for(int j = 1; j < SIZEY - 1; j++)
                for(int k = 1; k < SIZEZ - 1; k++)
                    TestDo3d7pkernel(itr,i,j,k);
    }
    for(int i = 1; i < SIZEX - 1; i++)
        for(int j = 1; j < SIZEY - 1; j++)
            for(int k = 1; k < SIZEZ - 1; k++)
            {
                if( fabs(test[(ITER+1)%2][i][j][k]  - A[(ITER+1)%2][i][j][k]) > 1e-4 )
                {
                    cout<<"i: "<<i<<"   j : " << j << "  k: "<<k<<endl;
                    cout<<"test: "<< test[(ITER+1)%2][i][j][k];
                    cout<<"\nA: "<< A[(ITER+1)%2][i][j][k];
                    cout<<"\nno"<<endl;
                    return -1;
                }
            }
    cout<<"correct\n";
    return 0;
}

#ifdef PAPI
#ifdef L3_CACHE
int total = 0;
void handler(int EventSet, void *address, long_long overflow_vector, void *context)
{
        fprintf(stderr, "handler(%d) Overflow at %p! vector=0x%llx\n",
                    EventSet, address, overflow_vector);
        total++;
}
#endif
#endif


int main(int argc, char** argv)
{
  struct timeval start;
  struct timeval end;
  double totaltime;


    if(argc > 5)
    {
        cout<<"input arg error"<<endl;
        return 0;
    }
    int thread_num = 1;
    int BLKX = 16;
    int BLKY = 16;
    int ITER = 1;
    if(argc > 1)
        ITER = atoi(argv[1]);
    if(argc > 2)
        BLKX = atoi(argv[2]);
    if(argc > 3)
        BLKY = atoi(argv[3]);
    if(argc > 4)
        thread_num = atoi(argv[4]);
	omp_set_num_threads(thread_num); 
    init();

    gettimeofday(&start,NULL);   
    Do3d7p(ITER, BLKX, BLKY, thread_num);
    gettimeofday(&end,NULL);
#ifdef CHECK
    if(check(ITER))
    {
        cout<<"exec time : "<< "check_error" << endl;
		exit(1);
    }
#endif
    totaltime=(end.tv_sec-start.tv_sec)+ (end.tv_usec-start.tv_usec)*1.0e-6;
    cout<<"exec time : "<<totaltime << endl;

    return 0;
}
