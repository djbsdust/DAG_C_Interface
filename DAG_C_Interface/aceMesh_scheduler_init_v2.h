#ifndef ACEMESH_SCHEDULER_INIT_H
#define ACEMESH_SCHEDULER_INIT_H

#include <cstddef>
#include "tbb/atomic.h"
#include <vector>


namespace AceMesh_runtime {

#ifdef __ACEMESH_THREAD_GROUP
class aceMesh_observer
{
private:
    std::vector<int> processor_ids;
    tbb::atomic<int> index;
    bool init;
    size_t n_groups;
    size_t group_size;

public:
    void set_thread_group_info(size_t n_groups, size_t group_size);

public :
    aceMesh_observer();
    ~aceMesh_observer();
    void set_info(int bind_processor_id[], int thread_num);
    void on_scheduler_entry(bool is_worker);
};
#endif

class aceMesh_scheduler_init 
{
public:
    int thread_num;
    aceMesh_scheduler_init();
    ~aceMesh_scheduler_init();
    void init_thread_num(int thread_num = 0);
    int thread_bind(int bind_processor_id[], int thread_num);
#ifdef __ACEMESH_THREAD_GROUP
    int thread_bind(int bind_core_ids[], size_t n_groups, size_t group_size);
    aceMesh_observer ob;
#endif
};


}

#endif
