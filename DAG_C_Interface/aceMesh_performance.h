#ifndef ACEMESH_PERFORMANCE_H
#define ACEMESH_PERFORMANCE_H

#include <fstream>
#include <string>
#include "tbb/concurrent_unordered_map.h"
#include "tbb/atomic.h"

/*
* PAPI is used to count Cache miss
*/

namespace AceMesh_runtime {
struct papi_thread_private_info {
  int event_set;
  long long values1[1];
  long long values2[1];  
    //other info
};
class aceMesh_performance
{
private:
    //std::string out_filename;

    tbb::concurrent_unordered_map<unsigned int, papi_thread_private_info*> papi_thread_info;
	 papi_thread_private_info* get_thread_info();
public:	
	long long record_start_papi_perf_cnt();
	long long record_end_papi_perf_cnt();
	long long get_record_papi_perf_cnt();
private:
   // thread_ctl my_output;
   // int event_set;
    //long long values1[1];
    //long long values2[1];
public:
    static void init();
    static void uninit();
    aceMesh_performance();
    ~aceMesh_performance();
    //void thread_init();
   // void record_start();
    //void record_end();
   //long long get_record_val();	
   // void record_output();

};
void acemesh_papi_init();
void acemesh_papi_uninit();
long long acemesh_papi_start_perf_cnt();
long long acemesh_papi_end_perf_cnt();
long long acemesh_papi_perf_cnt();
}


#endif
