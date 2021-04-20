#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cassert>
#include <sys/time.h>
#include <omp.h>
using namespace std;

//you had better not define SIZE directly here, but use env variables when make
//#define SIZE 256
double A[2][SIZEX][SIZEY]; //__attribute__ ((aligned(64)));
//for comparison with A array
double test[2][SIZEX][SIZEY];


//args
#define alpha_d 0.0876
#define beta_d  0.0765

//real kernel 
#define Do2d5pkernel(dest,src,i,j) {\
   A[dest][i][j] = alpha_d * (A[(src)][(i)][(j)]) + \
                        beta_d * (A[src][i-1][j] + A[src][i+1][j] +\
                        A[src][i][j-1] + A[src][i][j+1] +\
                        A[src][i][j] + A[src][i][j]);}
//for comparison
#define TestDo2d5pkernel(itr,i,j) {\
   test[itr%2][i][j] = alpha_d * test[(itr+1)%2][i][j] + \
                        beta_d * (test[(itr+1)%2][i-1][j] + test[(itr+1)%2][i+1][j] +\
                        test[(itr+1)%2][i][j-1] + test[(itr+1)%2][i][j+1] +\
                        test[(itr+1)%2][i][j] + test[(itr+1)%2][i][j] ); } 




int Do2d5p(const int ITER, const int blocsizex, const int blocsizey, int thread_num)
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
                    Do2d5pkernel(dest,src,ii,jj);
            }//j loop
    }//itr loop
    return 0;
}


void init()
{
    for(int i = 0; i < SIZEX; i++)
    for(int j = 0; j < SIZEY; j++)
       A[1][i][j] = (double)(i*2.5 + j*3.3 ) /3.0; 
}
int check(const int ITER)
{
    cout<<"begin check\n";
    for(int i = 0; i < SIZEX; i++)
    for(int j = 0; j < SIZEY; j++)
          test[1][i][j] = (double)(i*2.5 + j*3.3 ) /3.0; 
    for(int itr = 0; itr < ITER; ++itr)
    {
        for(int i = 1; i < SIZEX - 1; i++)
        for(int j = 1; j < SIZEY - 1; j++)
           TestDo2d5pkernel(itr,i,j);
    }
    for(int i = 1; i < SIZEX - 1; i++)
    for(int j = 1; j < SIZEY - 1; j++)
       if( fabs(test[(ITER+1)%2][i][j]  - A[(ITER+1)%2][i][j]) > 1e-4 )
       {
          cout<<"i: "<<i<<"   j : " << j <<endl;
          cout<<"test: "<< test[(ITER+1)%2][i][j];
          cout<<"\nA: "<< A[(ITER+1)%2][i][j];
          cout<<"\nno"<<endl;
          return -1;
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
    Do2d5p(ITER, BLKX, BLKY, thread_num);
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
