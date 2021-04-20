#include "aceMesh_scheduler_init_v2.h"

#include "scheduler.h"
#include "tbb_thread_local.h"

#ifdef ACEMESH_PERFORMANCE
#include "aceMesh_performance.h"
#endif

namespace AceMesh_runtime {
aceMesh_scheduler_init::aceMesh_scheduler_init()
{
#ifdef ACEMESH_PERFORMANCE
    //acemesh_papi_init();
#endif
}

aceMesh_scheduler_init::~aceMesh_scheduler_init()
{
    //generic_scheduler::close_worker_thread();
#ifdef ACEMESH_PERFORMANCE
    //acemesh_papi_uninit();
#endif
}
void aceMesh_scheduler_init::init_thread_num(int thread_num)
{
    AceMesh_runtime::generic_scheduler::create_worker_thread(thread_num);
    this->thread_num = thread_num;
}


int aceMesh_scheduler_init::thread_bind(int bind_processor_id[], int thread_num)
{
    AceMesh_runtime::generic_scheduler::create_worker_thread(thread_num, bind_processor_id);
    this->thread_num = thread_num;
//    std::cout<<"THREAD NUM..."<<this->thread_num<<std::endl;
//    int i=0;
//    for(i=0;i<3;i++)
//    std::cout<<"CORE ID..."<<bind_processor_id[i]<<std::endl;
    return 0;
}

#ifdef __ACEMESH_THREAD_GROUP
int aceMesh_scheduler_init::thread_bind(int bind_core_ids[], size_t n_groups, size_t group_size)
{
    ob.set_thread_group_info(n_groups, group_size);
    ob.set_info(bind_core_ids, n_groups * group_size);

    AceMesh_runtime::generic_scheduler::create_worker_thread(&ob, n_groups, bind_core_ids);
    this->thread_num = n_groups;
    return 0;
}
#endif


#ifdef __ACEMESH_THREAD_GROUP
aceMesh_observer::aceMesh_observer(): index(), init(false)
{
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


void aceMesh_observer::set_thread_group_info(size_t n_groups, size_t group_size)
{
    this->n_groups = n_groups;
    this->group_size = group_size;
}


void aceMesh_observer::on_scheduler_entry(bool is_worker)
{    
    int my_core_id;
    int my_id = index++;

    // get the core ids for sub-threads in the thread group.
    std::vector<size_t> cores_in_group;
    for (size_t i = 1; i < group_size; ++i)
    {
        cores_in_group.push_back(processor_ids[my_id * group_size + i]);
    }

    tbb_thread_local::get_subthread_group().init(my_id, group_size - 1, cores_in_group);
    tbb_thread_local::get_subthread_group().start();


    if(!init)
        return ;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(processor_ids[my_id * group_size], &mask);
    my_core_id = processor_ids[my_id * group_size]; 


    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    {
        std::cout << "pthread_setaffinity_np failed";
    }
    std::cout << pthread_self() << "   success bind to processor id : " << my_core_id << std::endl;
}
#endif

}
