#ifndef ACEMESH_UTILS
#define ACEMESH_UTILS

#include <ctime>
#include<stdlib.h>
namespace AceMesh_runtime {
inline long long tick_count_now()
{
    long long my_count;
    struct timespec ts;
    clock_gettime( CLOCK_REALTIME, &ts );
    my_count = static_cast<long long>(1000000000UL)*static_cast<long long>(ts.tv_sec) + static_cast<long long>(ts.tv_nsec);
    return my_count;
}

/** Defined in tbb_main.cpp **/
unsigned GetPrime ( unsigned seed );

//NUMA node(s)
const int NUMA_num = 2;
//Socket(s)
const int sockets = 2;
//Core(s) per socket
const int cores = 14;
//static const unsigned NUMA[NUMA_num][sockets*cores]={
//{0,1,2,3,4,5,6,7,8,9,10,11,12,13,28,29,30,31,32,33,34,35,36,37,38,39,40,41},
//{14,15,16,17,18,19,20,21,22,23,24,25,26,27,42,43,44,45,46,47,48,49,50,51,52,53,54,55}
//};
static const unsigned NUMA[NUMA_num][cores]={
{0,1,2,3,4,5,6,7,8,9,10,11,12,13},
{14,15,16,17,18,19,20,21,22,23,24,25,26,27}
};

//! A fast random number generator.
/** Uses linear congruential method. */
class FastRandom {
    unsigned x, a;
public:
    //! Get a random number.
    unsigned short getnum(int id,int num_threads){
    	//窃取的NUMA节点编号
		int tmp = (id/cores)%NUMA_num;

		if(((num_threads-1)/cores) % NUMA_num == tmp)
		{
			//可以窃取线程的数量,窃取出现未满的情况
			//也可以理解为num_thread和id在同一numa节点内
			int number = (num_threads/cores)/NUMA_num*cores+num_threads%cores;
			if(number == 0) 
				number = cores;
			if(number == 1)
				return NUMA[tmp][0];
			else
				return NUMA[tmp][rand() % number];
		}
		else
		{	
			//窃取NUMA满的情况
			int number =  (num_threads/cores)/NUMA_num*cores;
			if(number == 0) 
				number = cores;
			return NUMA[tmp][rand() % number];
		}
    }
    unsigned short get() {
        return get(x);
    }   
    //! Get a random number for the given seed; update the seed for next use.
    unsigned short get( unsigned& seed ) { 
        unsigned short r = (unsigned short)(seed>>16);
        seed = seed*a+1;
        return r;
    }   
    //! Construct a random number generator.
    FastRandom( unsigned seed ) { 
        x = seed;
        a = GetPrime( seed );
    }
};
}
#endif
