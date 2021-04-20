#ifndef _ACEMESH_SCHEDULER_INIT_H
#define _ACEMESH_SCHEDULER_INIT_H

#include "tbb/task_scheduler_observer.h"
#include <vector>
#include "tbb/atomic.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/enumerable_thread_specific.h"

namespace AceMesh_runtime {

class aceMesh_observer : public tbb::task_scheduler_observer
{
private:

#ifdef __ACEMESH_THREAD_GROUP
    size_t n_groups;
    size_t group_size;
public:
    void set_thread_group_info(size_t n_groups, size_t group_size);
private:

#endif

    bool init;
    std::vector<int> processor_ids;
    tbb::atomic<int> index;
public :
    aceMesh_observer();
    ~aceMesh_observer();
    void set_info(int bind_processor_id[], int thread_num);
    void on_scheduler_entry(bool is_worker);
    void on_scheduler_exit( bool is_worker);
};

class aceMesh_scheduler_init 
{
private:
    aceMesh_observer ob;
    tbb::task_scheduler_init init;
public:
    int thread_num;
    aceMesh_scheduler_init();
    ~aceMesh_scheduler_init();
    void init_thread_num(int thread_num = 0);
    int thread_bind(int bind_processor_id[], int thread_num);

#ifdef __ACEMESH_THREAD_GROUP
    int thread_bind(int bind_core_ids[], size_t n_groups, size_t group_size);
#endif

};

int get_thread_id();
}

//#ifdef ACEMESH_PERFORMANCE
//void print_papi_info();
//#endif

#endif
