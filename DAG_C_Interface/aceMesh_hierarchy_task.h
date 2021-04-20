#ifndef _ACEMESH_HIERARCHY_TASK_H_
#define _ACEMESH_HIERARCHY_TASK_H_


#include <memory>
#include "tbb/tick_count.h"

#include "thread_group.h"
#include "splitter.h"
#include "tbb_thread_local.h"

namespace AceMesh_runtime {

template<typename TaskBody, typename RangeType> class aceMesh_hierarchy_task: public hierarchy_task_base //, public aceMesh_task
{

	RangeType base_range;
	std::shared_ptr<TaskBody> p_task_body;	// NEED DISCUSSION: pointer or object?

public:
	
	aceMesh_hierarchy_task()
	{
	}

	aceMesh_hierarchy_task(TaskBody* _p_task_body, const RangeType & _base_range): base_range(_base_range), p_task_body(_p_task_body)
	{
	}

	virtual ~aceMesh_hierarchy_task()
	{
	}

	aceMesh_hierarchy_task(const aceMesh_hierarchy_task & other)
	{
		this->base_range = other.base_range;
		this->p_task_body = other.p_task_body;
	}
	virtual void execute_task_body(const void* p_param);

	virtual task* execute();
#ifndef SWF
	virtual void get_neighbor(int neighbor, void* src_addr, std::vector<void*>& neighbor_addrs);
#else
	virtual void get_neighbor( );
#endif

};

template<typename TaskBody, typename RangeType> task* aceMesh_hierarchy_task<TaskBody, RangeType>::execute()
{

	thread_group& sub_thread_group = tbb_thread_local::get_subthread_group();
	splitter& my_splitter = tbb_thread_local::get_splitter();

	std::vector<RangeType> splitted_ranges;
	my_splitter.split(base_range, splitted_ranges);

	// the last chunk of work is processed by the master thread itself.
	// get the last chunk out of the ranges which will be submitted to sub-threads.
	RangeType last_one_range = splitted_ranges.back();
	splitted_ranges.pop_back();


	// start the sub-threads to do their work.
	assert(splitted_ranges.size() == sub_thread_group.get_group_size());
	for (size_t i = 0; i < sub_thread_group.get_group_size(); ++i)
	{
		sub_thread_group.set_task_param(i, &splitted_ranges[i]);
		sub_thread_group.set_task(i, this);
		sub_thread_group.run_task(i);
	}

	// do the last chunk of work myself.
	(*p_task_body)(last_one_range);

	sub_thread_group.wait_all_tasks();

	return aceMesh_task::execute();

}

template<typename TaskBody, typename RangeType> void aceMesh_hierarchy_task<TaskBody, RangeType>::execute_task_body(const void* p_param)
{
	(*p_task_body)(*(RangeType*)p_param);
}

#ifdef SWF
template<typename TaskBody, typename RangeType> void aceMesh_hierarchy_task<TaskBody, RangeType>::get_neighbor(int neighbor, void* src_addr, std::vector<void*>& neighbor_addrs)
{
	(*p_task_body).get_neighbor(neighbor, src_addr, neighbor_addrs);
}
#else
template<typename TaskBody, typename RangeType> void aceMesh_hierarchy_task<TaskBody, RangeType>::get_neighbor( ) {
	(*p_task_body).get_neighbor( );
}

#endif
}
#endif
