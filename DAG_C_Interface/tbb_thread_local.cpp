#include "tbb_thread_local.h"

extern AceMesh_runtime::splitter cur_splitter;
namespace AceMesh_runtime {
tbb_thread_local::thread_local_storage_t tbb_thread_local::thread_local_storage;


tbb_thread_local::tbb_thread_local()
{
    // make a local copy of the global splitter, so the local splitter dosen't need to be locked when being accessed.
    my_splitter = cur_splitter;
}

thread_group& tbb_thread_local::get_subthread_group()
{
	return thread_local_storage.local().subthread_group;
}

splitter& tbb_thread_local::get_splitter()
{
	return thread_local_storage.local().my_splitter;
}
}
