#ifndef ACEMESH_STACK_H
#define ACEMESH_STACK_H

#include "task.h"
//#include "tbb/concurrent_vector.h"
#include "tbb/critical_section.h"
#include "tbb/atomic.h"

#include <stack>

namespace AceMesh_runtime {
//only support one pop and multi push
//two and more pop will have error
//but in our system, task only push and pop one time, so it is OK!
//to learn more: google "ABA problem"
class concurrent_aceMesh_stack
{
public:
    concurrent_aceMesh_stack();
    ~concurrent_aceMesh_stack();
    void push(task* t);
    bool try_push(task* t);
    void pop(task*& t);
    bool try_pop(task*& t);
    bool empty();
private:
    tbb::atomic<task*> task_pool;
};

/*
class concurrent_stack
{
private:
    std::stack<task*> task_pool;
    tbb::critical_section cs;
public:
    concurrent_stack();
    ~concurrent_stack();
    void push(task* t);
    bool try_pop(task*& t);
};
*/

/****************************************************
 * This data structure is not secure for concurrency
 ****************************************************/
class concurrent_aceMesh_queue
{
public:
    concurrent_aceMesh_queue();
    ~concurrent_aceMesh_queue();
    void push(task* t);
    bool try_pop(task*& t);
private:
    struct node 
    {
        task* t;
        tbb::atomic<node*> next;
        node():t(NULL), next()
        {}
        node(task* my):t(my), next()
        {}
    };

    tbb::atomic<node*> head;
    tbb::atomic<node*> tail;
    //tbb::atomic<int> my_len;
};
	
class aceMesh_queue
{
public:
    aceMesh_queue();
    ~aceMesh_queue();
    void push(task* t);
    bool try_pop(task*& t);
private:
    struct node 
    {
        task* t;
        tbb::atomic<node*> next;
        node():t(NULL), next()
        {}
        node(task* my):t(my), next()
        {}
    };

    tbb::atomic<node*> head;
    tbb::atomic<node*> tail;
    //tbb::atomic<int> my_len;
};

#ifdef MTEST_LIGHT
class aceMesh_mtest_queue
{
public:
    aceMesh_mtest_queue();
    ~aceMesh_mtest_queue();
    void push(mtest_task* t);
    bool try_pop(mtest_task*& t);
    tbb::atomic<mtest_task*> head;
    tbb::atomic<mtest_task*> tail;
//private:
//    struct node 
//    {
//        mtest_task* t;
//        tbb::atomic<node*> next;
//        node():t(NULL), next()
//        {}
//        node(mtest_task* my):t(my), next()
//        {}
//    };

//    tbb::atomic<node*> head;
//    tbb::atomic<node*> tail;
//    //tbb::atomic<int> my_len;
};
#endif


}

#endif
