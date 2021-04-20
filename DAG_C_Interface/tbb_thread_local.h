#ifndef _ACEMESH_TBB_THREAD_LOCAL_H_
#define _ACEMESH_TBB_THREAD_LOCAL_H_


#include "thread_group.h"
#include "splitter.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/tick_count.h"

namespace AceMesh_runtime {
class tbb_thread_local
{
	thread_group subthread_group;
	splitter my_splitter;

	typedef tbb::enumerable_thread_specific<tbb_thread_local> thread_local_storage_t;
	static thread_local_storage_t thread_local_storage;

public:
	tbb_thread_local();

	static thread_group& get_subthread_group();
	static splitter& get_splitter();

};
}
#endif
