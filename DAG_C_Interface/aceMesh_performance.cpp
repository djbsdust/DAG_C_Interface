#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdarg>


#include "trace_out.h"
#include "aceMesh_runtime.h"
#include "scheduler.h"
#include "aceMesh_performance.h"
#include "papi.h"

using namespace std;
using namespace tbb;

namespace AceMesh_runtime {

//thread_ctl aceMesh_performance::my_output("aceMesh_performance/aceMesh_performance_");
aceMesh_performance aceMesh_papi_performance_counter;
long long acemesh_papi_start_perf_cnt() {  
	return aceMesh_papi_performance_counter.record_start_papi_perf_cnt();
}
long long acemesh_papi_end_perf_cnt() {
	return aceMesh_papi_performance_counter.record_end_papi_perf_cnt();
}
long long acemesh_papi_perf_cnt() {
	return aceMesh_papi_performance_counter.get_record_papi_perf_cnt();
}
void acemesh_papi_init() {
  aceMesh_performance::init();
}
void acemesh_papi_uninit() {
  aceMesh_performance::uninit();
}
#define PAPI_FORCE PAPI_L3_TCM
//#define PAPI_FORCE PAPI_TLB_DM
papi_thread_private_info* aceMesh_performance::get_thread_info()
{
    unsigned int myid=generic_scheduler::theTLS.get()->get_myid();
    concurrent_unordered_map<unsigned int, papi_thread_private_info*>::iterator itr = papi_thread_info.find(myid);
    
	if ( itr == papi_thread_info.end() ) {
        papi_thread_private_info *info = new papi_thread_private_info();
         info->event_set = PAPI_NULL;
        int retval = PAPI_create_eventset (&info->event_set);
        assert(retval==PAPI_OK);
        retval = PAPI_add_event(info->event_set, PAPI_FORCE);
        //retval = PAPI_add_event(info->event_set, PAPI_L3_TCM);
        assert(retval==PAPI_OK);

        //retval = PAPI_attach(info->event_set, getpid());
        //assert(retval==PAPI_OK);
        
        std::pair<unsigned int , papi_thread_private_info*> ins(myid, info);
        papi_thread_info.insert(ins);
        
        return info;
    }
    else
    {
        return itr->second;
    }
}


long long aceMesh_performance::record_start_papi_perf_cnt() {
  papi_thread_private_info *info = get_thread_info();
        
 int retval = PAPI_start (info->event_set);
 assert(retval==PAPI_OK);

 retval = PAPI_read(info->event_set, info->values1);
 assert(retval==PAPI_OK);

 return info->values1[0];    
}
long long aceMesh_performance::record_end_papi_perf_cnt() {
   papi_thread_private_info *info = get_thread_info();
   int retval = PAPI_stop (info->event_set,info->values2);
   assert(retval==PAPI_OK);
   return info->values2[0];
}
long long aceMesh_performance::get_record_papi_perf_cnt() {
   papi_thread_private_info *info = get_thread_info();
   return (info->values2[0] - info->values1[0]);

}
void aceMesh_performance::init()
{
    if (PAPI_library_init(PAPI_VER_CURRENT) !=
        PAPI_VER_CURRENT)
        exit(1);
   // int status =  mkdir("aceMesh_performance", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //std::cout << "status: " << status <<std::endl;
    //TODO

    // support multi-threads
    if (PAPI_thread_init(pthread_self) != PAPI_OK)
       exit(1);
}

void aceMesh_performance::uninit()
{
    /* Shutdown PAPI */
    PAPI_shutdown();
}

aceMesh_performance::aceMesh_performance()
{
  //my_output.init("aceMesh_performance/aceMesh_performance_");
}


aceMesh_performance::~aceMesh_performance()
{
    for(concurrent_unordered_map<unsigned int, papi_thread_private_info*>::iterator itr = papi_thread_info.begin();
        itr != papi_thread_info.end(); ++itr)
    {
      
    // int retval = PAPI_detach(itr->second->event_set);
     //printf("PAPI_detach retval=%d\n", retval);
     //assert(retval==PAPI_OK);
 
    /* Clean up EventSet */
    if (PAPI_cleanup_eventset(itr->second->event_set) != PAPI_OK)   
        exit(-1);
    /* Destroy the EventSet */
    if (PAPI_destroy_eventset(&itr->second->event_set) != PAPI_OK)  
        exit(-1);
        delete itr->second;
    }
}
/*void aceMesh_performance::thread_init()
{
}*/

//void aceMesh_performance::record_start()
//{
    //my_output.print("L3 cache miss : ");
    //long long thold = 1<<31ll;

    //retval = PAPI_overflow(event_set, PAPI_L3_TCM, 
    //    thold, 0, handler);
    //
//    int retval = PAPI_start (event_set);
//    assert(retval==PAPI_OK);
//    PAPI_read(event_set, values1);
//    assert(retval==PAPI_OK);
//}
//void aceMesh_performance::record_end()
//{
//    int retval = PAPI_stop (event_set,values2);
//    assert(retval==PAPI_OK);
//}

//long long aceMesh_performance::get_record_val() {
//	return (values2[0] - values1[0]);
//}
//void aceMesh_performance::record_output()
//{   
//    my_output.print("PAPI FORCE : ");
//    my_output.print_long_long(values2[0] - values1[0]);
//    print_to_internal_thread_file("\n");
//}

}
