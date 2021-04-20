#include "aceMesh_deque.h"
#include <cassert>
namespace AceMesh_runtime {
/******
*
* This data structure is not secure for concurrency.
*

aceMesh_deque::aceMesh_deque(): task_queue( new circular_array<task*>(TASKQUEUE_INIT_SIZE) ) 
{
    top = 0;
    bottom = 0;
}

aceMesh_deque::~aceMesh_deque()
{
    delete task_queue;
}


void aceMesh_deque::push(task*& my_task)
{
    size_t t = this->top;
    size_t b = this->bottom;

    long size = b - t;

    circular_array<task*>* old_task_queue = task_queue;
    if(__builtin_expect( size >= task_queue->my_size - 1, 0) )
    {
        std::cout << "may be bug in here\n"; 
        task_queue = task_queue->get_double_sized_copy(b, t);
        delete old_task_queue;
    }

    task_queue->set(b, my_task);
    ++this->bottom;
}

bool aceMesh_deque::try_pop(task*& my_task)
{
    my_task = NULL;
    long long b = this->bottom;
    b = b - 1;
    --this->bottom;

    long long t = this->top;
    long long size = b - t;
    if(__builtin_expect( size < 0, 0) )
    {
        bottom = t;
        return false;
    }
    assert( b >= 0 );
    my_task = task_queue->get(b);
    if(size > 0)
    {
        return true;
    }
    if( this->top.compare_and_swap(t+1, t) != t)
    {
        my_task = NULL;
        this->bottom = t + 1;
        return false;
    }
    this->bottom = t + 1;
    return true;
}

task* aceMesh_deque::take()
{
    long long t = this->top;
    long long b = this->bottom;

    long long size = b - t;

    if(size <= 0 ) return NULL;

    if( top.compare_and_swap(t+1, t) == t)
    {
        //std::cout << "take a task\n";
        return task_queue->get(t);
    }
    return NULL;
}
*****/
}
