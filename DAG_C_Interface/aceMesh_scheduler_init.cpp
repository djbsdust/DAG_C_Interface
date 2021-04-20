#include "aceMesh_scheduler_init.h"
#include <tbb/tbb_thread.h>
#include <iostream>

#include "thread_group.h"
#include "tbb_thread_local.h"

#ifdef ACEMESH_PERFORMANCE
#include "aceMesh_performance.h"
#endif

namespace AceMesh_runtime {

#ifndef __ACEMESH_THREAD_GROUP
typedef tbb::enumerable_thread_specific<int> aceMesh_TLS;
aceMesh_TLS tls(0);
#endif



//typedef tbb::enumerable_thread_specific<aceMesh_performance*> aceMesh_papi_TLS;
//aceMesh_papi_TLS papi_tls;
//void print_papi_info()
//{
//    for(auto itr = papi_tls.begin(); itr != papi_tls.end(); ++itr)
//    {
//        (*itr)->record_end();
//        (*itr)->record_output();
//    }
//}
//#endif

aceMesh_observer::aceMesh_observer(): index(), init(false)
{
    observe(true);
}
aceMesh_observer::~aceMesh_observer()
{
}

void aceMesh_observer::set_info(int bind_processor_id[], int thread_num)
{
    for(int i = 0; i < thread_num; ++i)
    {
        processor_ids.push_back(bind_processor_id[i]);
    }
    init = true;
}

#ifdef __ACEMESH_THREAD_GROUP
void aceMesh_observer::set_thread_group_info(size_t n_groups, size_t group_size)
{
    this->n_groups = n_groups;
    this->group_size = group_size;
}
#endif

void aceMesh_observer::on_scheduler_entry(bool is_worker)
{    
    int my_core_id;
    int my_id = index++;

#ifdef __ACEMESH_THREAD_GROUP
    // get the core ids for sub-threads in the thread group.
    std::vector<size_t> cores_in_group;
    for (size_t i = 1; i < group_size; ++i)
    {
        cores_in_group.push_back(processor_ids[my_id * group_size + i]);
    }

    tbb_thread_local::get_subthread_group().init(my_id, group_size - 1, cores_in_group);
    tbb_thread_local::get_subthread_group().start();

#else
    aceMesh_TLS::reference thread_id = tls.local();
    thread_id = my_id;
    
//#ifdef ACEMESH_PERFORMANCE
//    aceMesh_papi_TLS::reference my_perf = papi_tls.local();
//    my_perf = new aceMesh_performance();
//    my_perf->thread_init();
//    my_perf->record_start();
//#endif

#endif

    if(!init)
        return ;
    cpu_set_t mask;
    CPU_ZERO(&mask);

#ifdef __ACEMESH_THREAD_GROUP
    CPU_SET(processor_ids[my_id * group_size], &mask);
    my_core_id = processor_ids[my_id * group_size]; 
#else
    CPU_SET(processor_ids[my_id], &mask);
    my_core_id = processor_ids[my_id];
#endif

    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    {
        std::cout << "pthread_setaffinity_np failed";
    }
    std::cout << pthread_self() << "   success bind to processor id : " << my_core_id << std::endl;
}

//TODO
//NOT WORK IN TBB
void aceMesh_observer::on_scheduler_exit( bool is_worker )
{
//#ifdef ACEMESH_PERFORMANCE
//    aceMesh_papi_TLS::reference my_perf = papi_tls.local();
//    my_perf->record_end();
//    my_perf->record_output();
//#endif
}

aceMesh_scheduler_init::aceMesh_scheduler_init(): ob(), init(tbb::task_scheduler_init::deferred)
{
#ifdef ACEMESH_PERFORMANCE
    //acemesh_papi_init();
#endif
}

aceMesh_scheduler_init::~aceMesh_scheduler_init()
{
#ifdef ACEMESH_PERFORMANCE
    //acemesh_papi_uninit();
#endif
}
void aceMesh_scheduler_init::init_thread_num(int thread_num)
{
#ifdef __ACEMESH_THREAD_GROUP
    ob.set_thread_group_info(thread_num, 1);
#endif
    init.initialize(thread_num);
    this->thread_num = thread_num;
}

int aceMesh_scheduler_init::thread_bind(int bind_processor_id[], int thread_num)
{
#ifdef __ACEMESH_THREAD_GROUP
    ob.set_thread_group_info(thread_num, 1);
#endif
    ob.set_info(bind_processor_id, thread_num);
    init.initialize(thread_num);
    this->thread_num = thread_num;
//    std::cout<<"THREAD NUM : "<<this->thread_num<<",CPUID :"<<bind_processor_id<<std::endl;
    return 0;
}

int get_thread_id()
{
    int id;
#ifdef __ACEMESH_THREAD_GROUP
    id = tbb_thread_local::get_subthread_group().get_group_id();
#else
    aceMesh_TLS::reference thread_id = tls.local();
    id = thread_id;
#endif
    return id;
}

#ifdef __ACEMESH_THREAD_GROUP
int aceMesh_scheduler_init::thread_bind(int bind_core_ids[], size_t n_groups, size_t group_size)
{
    ob.set_thread_group_info(n_groups, group_size);
    ob.set_info(bind_core_ids, n_groups * group_size);
    init.initialize(n_groups);
    this->thread_num = n_groups;
    return 0;
}
#endif
}
