#include "task.h"
#include "scheduler.h"

namespace AceMesh_runtime {
task::task():affinity_id(NO_SET_AFFINITY),ref_count_t(), next(NULL), extra_state(0), priority_id(0), spawn_order_id(0),stored(false)
{
}
task::task(int id):affinity_id(id),ref_count_t(), next(NULL), extra_state(0), priority_id(0), spawn_order_id(0),stored(false)
{
}


task::~task()
{
}

//mtest_task::mtest_task():comm_handle1(NULL),comm_handle2(NULL),comm_task(NULL)
//{
//}
//mtest_task::mtest_task(int &id1, int &id2, task& t):comm_handle1(id1),comm_handle2(id2),comm_task(t)
//{
//}
//mtest_task::~mtest_task()
//{
//}


void task::wait_for_all()
{
    generic_scheduler::main_kernel_func(this);
}


void task::spawn(std::vector<task*>& init_task_list)
{
    for(std::vector<task*>::iterator itr = init_task_list.begin(); itr != init_task_list.end(); ++itr)
    {
        generic_scheduler::theTLS.get()->init_spawn(*itr);
    }
}
void task::init_spawn(task& t)
{
    generic_scheduler::theTLS.get()->init_spawn(&t);
}

void task::spawn(task& t)
{
    generic_scheduler::theTLS.get()->spawn_to_id(&t);

    //generic_scheduler::theTLS.get()->local_spawn(&t);
    /*
    if(t.get_affinity_id() > -1)
    {
        generic_scheduler::theTLS.get()->local_spawn(&t);
        //generic_scheduler::theTLS.get()->spawn_to_id(&t);
    }
    else
    {
        //generic_scheduler::theTLS.get()->local_spawn(&t);
        generic_scheduler::theTLS.get()->enqueue(&t);
    }*/
}
#ifndef NOT_MPI_STRATEGY
#ifdef EXTRA_THREAD //
void task::mpi_spawn(task& t)
{
	  generic_scheduler::mpi_scheduler->mpi_local_spawn(&t);
     
}
#ifdef MTEST_LIGHT
void task::mpi_polling_spawn(task& t)
{
	  generic_scheduler::mpi_scheduler->mpi_polling_local_spawn(&t);
     
}
#endif

#endif
#ifdef BLOCKING_QUEUE  //add wangm
void task::suspend_spawn(task& t)
{
     generic_scheduler::theTLS.get()->mpiqueue(&t);
}
#endif
#endif
void task::enqueue(task& t)
{
    generic_scheduler::theTLS.get()->enqueue(&t);
}

void task::adjust_affinity_id(task* another_task)
{
    if(this->affinity_id == INIT_AFFINITY)
    {
        this->affinity_id != another_task->affinity_id;
        return;
    }
    if(this->affinity_id != another_task->affinity_id)
    {
        this->affinity_id = FOLLOW_AFFINITY;
    }
}

}
