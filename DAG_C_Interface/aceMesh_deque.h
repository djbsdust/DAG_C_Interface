#ifndef ACEMESH_DEQUE_H
#define ACEMESH_DEQUE_H

#include "task.h"
#include "circular_array.h"

/* Constants */
#define TASKQUEUE_INIT_SIZE 1310720 
//#define TASKQUEUE_INIT_SIZE 4096
namespace AceMesh_runtime {
/****
*
* This data structure is not secure for concurrency.
*

class aceMesh_deque
{
private:
    circular_array<task*>* task_queue;
    tbb::atomic<size_t> top;
    tbb::atomic<size_t> bottom;

public:
    aceMesh_deque();
    ~aceMesh_deque();
    void push(task*& t);
    bool try_pop(task*& t);
    task* take();
};
*****/
}

#endif
