#ifndef _CONCURRENT_ACE_MESH_TASK_
#define _CONCURRENT_ACE_MESH_TASK_

#include <vector>

#if defined(DYNAMIC_SCHEDULER)
#include <tbb/task.h>
using namespace tbb;
#elif defined(ACEMESH_SCHEDULER)
#include "task.h"
#endif

#include <tbb/tick_count.h>
#include <string>
#include "tbb/atomic.h"
#include "tbb/concurrent_unordered_set.h"

#include <iostream>

namespace AceMesh_runtime {
class concurrent_aceMesh_task: public task {

private:
	tbb::concurrent_unordered_set<task*> successor_tasks;
	tbb::atomic<task*> vertical_task;
public:
	concurrent_aceMesh_task();
    virtual ~concurrent_aceMesh_task();

	virtual task* execute();
#ifndef
    virtual void get_neighbor(int neighbor, void* src_addr, std::vector<void*>& neighbor_addrs);
#else
virtual void get_neighbor( );
#endif
	void add_successor(task* t);
	void set_vertical_task(task* t);
#ifdef ACEMESH_SCHEDULER_PROFILING
    static void print_and_reset_reuse_statistics();
#endif

#ifdef ACEMESH_TIME
public:
    static void print_and_reset_execute_time();
protected:
	virtual void record_execute_time();
private:
	tbb::tick_count start_t;
#endif

};
}

#endif
