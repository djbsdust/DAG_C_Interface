#ifndef _ACE_MESH_RUNTIME_
#define _ACE_MESH_RUNTIME_


#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "aceMesh_runtime_c.h"
#include "task.h"
//interface of acemesh_task
#ifdef  NO_PARALLEL
	#ifdef CONCURRENT_CONSTRUCT_GRAPH
	#include "aceMesh_concurrent_task.h"
	#else
	#include "aceMesh_task.h"
	#endif

//	typedef AceMesh_runtime::aceMesh_task aceMesh_task;
#else
	#include "concurrent_aceMesh_task.h"
//	typedef AceMesh_runtime::concurrent_aceMesh_task aceMesh_task;
#define aceMesh_task concurrent_aceMesh_task

#endif

#include "tbb/enumerable_thread_specific.h"

#ifdef MEMORY_POOL
#include "MemPool.h"
#endif


using namespace AceMesh_runtime;

#if defined(DYNAMIC_SCHEDULER)
#define ACEMESH_NEW  new ( tbb::task::allocate_root() )
#elif defined(ACEMESH_SCHEDULER)
#define ACEMESH_NEW  new 
#endif

#ifdef __ACEMESH_THREAD_GROUP
// interfaces for hierarchical execution
Error_Code aceMesh_runtime_init(int total_threads, int group_size, int core_ids[]);
Error_Code aceMesh_runtime_init(int total_threads, int group_size);
Error_Code aceMesh_runtime_init(int total_threads);
Error_Code aceMesh_runtime_init(int total_threads, int core_ids[]);
Error_Code aceMesh_runtime_init();
void set_splitter_dim_chunks( size_t dim, ... );
#include "range.h"

typedef AceMesh_runtime::range_d range_d;
#endif
	Error_Code aceMesh_runtime_init(int thread_num);
	Error_Code aceMesh_runtime_init(int thread_num, int core_ids[]);
	Error_Code aceMesh_runtime_init();

	Error_Code aceMesh_runtime_shutdown();
	Error_Code aceMesh_runtime_shutdown_with_proc_id(int proc_id);
//aceMesh interface for get thread num in internal scheduler
int get_thread_num();

//n void* addr, short int area, int neighbor, int inout_type, int neighbor_inout_type
//eg: A[0], NORMAL, NEIGHBOR_ALL, INOUT, IN
//eg  B[0], SHADE_AND_UNSHADE, NEIGHBOR_NONE, INOUT, INOUT_NONE, (SHADE)
//                           NEIGHBOR_ALL,  INOUT_NONE, IN   , (UNSHADE)
//eg  C[0], SHADE, NEIGHBOR_NONE, INOUT, INOUT_NONE, (SHADE)
//eg  D[0], UNSHADE, NEIGHBOR_NONE, INOUT, INOUT_NONE, (UNSHADE)
Error_Code register_task(aceMesh_task* t, int n, ...);
Error_Code register_task_datainfo(aceMesh_task* t, int n, ...);
Error_Code register_task_end(aceMesh_task* t);


#ifdef __ACEMESH_THREAD_GROUP
// fine-grain task interface.
template<typename TaskBody, typename RangeType>
Error_Code register_task(TaskBody* task_body, const RangeType& range, int n, ...);
#endif
Error_Code dag_start(int dagNo, int *int_vec, int n1, double *float_vec, int n2);
Error_Code begin_split_task(const std::string& loop_info);

#if defined(AUTO_PARTITION) || defined(SUPPORT_PARTITION)
void spawn_and_wait_sep();
#endif

//void spawn();
//void wait();
extern int num_registers;   
#define SERIALIZE_POSTWAIT      0x1111UL
int aceMesh_get_thread_id();

//debug interface
void print_to_thread_file(const char* format, ... );

// template implementation.
#ifdef __ACEMESH_THREAD_GROUP
#include <cstdarg>
#include "aceMesh_hierarchy_task.h"


void do_register(aceMesh_task* p_task, int n, va_list* p_args);

template<typename TaskBody, typename RangeType> Error_Code register_task(TaskBody* task_body, const RangeType& range, int n, ...)
{

    aceMesh_task* hierarchy_task = ACEMESH_NEW AceMesh_runtime::aceMesh_hierarchy_task<TaskBody, RangeType>(task_body, range);

	va_list args;           
	va_start(args,n); 
	do_register(hierarchy_task, n, &args);
    //sort(addrs.begin(), addrs.end(), )
    va_end(args);
    
    return ACEMESH_OK;
}
#endif

//void set_separations(std::map<void*, int>& sep_datas);
#ifdef SUPPORT_PARTITION
void __acemesh_setsepid(int id);
#endif


void* aceMesh_get_data();
void aceMesh_set_data(void* data);
int get_part_id();
void set_part_id(int part_id);

namespace AceMesh_runtime {
  extern tbb::enumerable_thread_specific<task*> cur_task; 
}
#endif
