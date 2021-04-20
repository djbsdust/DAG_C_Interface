#ifndef _ACEMESH_THREAD_GROUP_H_
#define _ACEMESH_THREAD_GROUP_H_


#include <vector>
#include <pthread.h>
#include <mutex>


#ifdef  NO_PARALLEL
#include "aceMesh_task.h"
#else
#include "concurrent_aceMesh_task.h"
using namespace AceMesh_runtime;
typedef concurrent_aceMesh_task aceMesh_task;
#endif

namespace AceMesh_runtime {
// abstract base class for the tasks that the sub-threads will work on.
// put it here to minimize the number of header files that users would see.
class hierarchy_task_base: public aceMesh_task
{
public:
	virtual void execute_task_body(const void* p_param) =0;
};


class thread_group
{
	size_t group_size;
	size_t group_id;

	// struct for controlling individual sub-threads in this thread group.
	struct thread_control_struct
	{
		// a bunch of ids.
		thread_group* p_my_group;
		pthread_t my_pthread_id;
//		size_t my_id;
		size_t my_core_id;

		// controlling facilities.
		bool is_idle;
		pthread_cond_t cond_go;
		pthread_cond_t cond_finish;
		pthread_mutex_t cond_wait_mutex;

		// variables for passing infomation to a sub-thread.
		const void* p_param;
		hierarchy_task_base* p_task;
	};

	std::vector<thread_control_struct> subthread_control_structs;
	// set it to false to end all the running sub-threads of this thread group.
	bool subthreads_run;

	// private constructors, not implemented.
	thread_group(const thread_group & other);
	const thread_group & operator= (const thread_group & other);


public:
	thread_group();
	~thread_group();

	void init(size_t group_id, size_t _group_size, const std::vector<size_t> & core_ids);
	void start();

	size_t get_group_size();
	size_t get_group_id();

	// set the task body.
	void set_task(size_t subthread_index, hierarchy_task_base* p_task);
	// set task param
	void set_task_param(size_t subthread_index, const void * p_param);

	// pass a pointer to the base class aceMesh_task.
	// task body is called by virtual function in aceMesh_task.
	void run_task(size_t subthread_index, hierarchy_task_base* p_task, const void* p_param);
	// should be called after task parameter and body are properly set.
	void run_task(size_t subthread_index);
	// void run_task_sync(size_t subthread_index);

	void wait_all_tasks();

	friend void* subthread_func(void* arg);


};
}
#endif
