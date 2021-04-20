#include "concurrent_aceMesh_task.h"
#include <cassert>

namespace AceMesh_runtime {
concurrent_aceMesh_task::concurrent_aceMesh_task():vertical_task()
{
    successor_tasks.clear();
}

concurrent_aceMesh_task::~concurrent_aceMesh_task()
{

}
#ifndef
void concurrent_aceMesh_task::get_neighbor(int neighbor, void* src_addr, std::vector<void*>& neighbor_addrs)
{
    return;
}
#else
void concurrent_aceMesh_task::get_neighbor( )
{
    return;
}

#endif
void concurrent_aceMesh_task::add_successor(task* t)
{
    if(t == NULL) return ;
	if(vertical_task == t)
        return ;
    std::pair<tbb::concurrent_unordered_set<task*>::iterator,
        bool> result = successor_tasks.insert(t);
	if(result.second)
        t->increment_ref_count();
}

void concurrent_aceMesh_task::set_vertical_task(task* t)
{
    if( t == NULL ) return ;
	if(vertical_task == t)
		return;
	tbb::concurrent_unordered_set<task*>::iterator itr = successor_tasks.find(t);
    if(itr != successor_tasks.end())
    {
        std::cout<<"warining! I haven't handle this situation"<< std::endl;
        return ;
    }
    if(vertical_task != NULL)
    {
	    successor_tasks.insert(vertical_task);
        //std::cout << "warning replace v_task" << std::endl;
    }
	vertical_task = t;
	t->increment_ref_count();
}

task* concurrent_aceMesh_task::execute() 
{
    for(tbb::concurrent_unordered_set<task*>::iterator itr = this->successor_tasks.begin();
        itr != this->successor_tasks.end(); ++itr)
	{
		if(task* t = *itr) 
		{
			if(t->decrement_ref_count() == 0)
				task::spawn(*t);
		}
	}

    if(vertical_task != NULL)
	    if(vertical_task->decrement_ref_count() == 0)
        {
		    return vertical_task;
        }
	return NULL;
}
#ifdef ACEMESH_TIME
void concurrent_aceMesh_task::record_execute_time() 
{
	    this->start_t = tbb::tick_count::now();
}
void concurrent_aceMesh_task::print_and_reset_execute_time()
{
    //TODO:  =>cerr
	std::cout<<"sorry that print_and_reset_execute_time() has not been realized under parallel register mode!"<<std::endl;
	return;
}
#endif
#ifdef  ACEMESH_SCHEDULER_PROFILING
void concurrent_aceMesh_task::print_and_reset_reuse_statistics()
{
    //TODO:  =>cerr
	std::cout<<"sorry that print_and_reset_reuse_statistics() has not been realized under parallel register mode!"<<std::endl;
	return;
}
#endif
}
