#include "aceMesh_stack.h"
#include <iostream>
#include <cassert>
namespace AceMesh_runtime {
concurrent_aceMesh_stack::concurrent_aceMesh_stack(): task_pool()
{
    //tail = 0;
    //task_pool_size = 0;
}
concurrent_aceMesh_stack::~concurrent_aceMesh_stack()
{
}


bool concurrent_aceMesh_stack::try_push(task* t)
{
    task* old_top = task_pool;
    t->next = old_top;
    if( task_pool.compare_and_swap(t, old_top) == old_top)
    {
        return true;
    }
    return false;
}

void concurrent_aceMesh_stack::push(task* t)
{
    task* old_top;
    do
    {
        old_top = task_pool;
        t->next = old_top;
    }while(task_pool.compare_and_swap(t, old_top) != old_top);

    //cs.lock();
    //task_pool.push(t);
    //cs.unlock();
    /*
    if( tail < task_pool_size)
    {
        task_pool[tail++] = t;
    }
    else
    {
        task_pool.push_back(t);
        tail = ++task_pool_size;
    }*/
}


bool concurrent_aceMesh_stack::try_pop(task*& t)
{
    task* old_top = task_pool;
    if(old_top == NULL) return false;
    task* new_top = old_top->next;
    if(task_pool.compare_and_swap(new_top, old_top) == old_top)
    {
        t = old_top;
        return true;
    }
    else 
    {
        t = NULL;
        return false;
    }
    /*
    t = NULL;
    if( cs.try_lock() )
    {
        if(task_pool.empty())
        {
            cs.unlock();
            return false;
        }    
        t = task_pool.top();
        task_pool.pop();
        cs.unlock();
        return true;
    }
    else 
        return false;
    */
    /*
    int tmp = --tail;
    if( tmp < 0) return false;
    t = task_pool[tmp];
    return true;
    */
}

//TODO
bool concurrent_aceMesh_stack::empty()
{
    return true;
    //return tail == 0;
}


//TODO
void concurrent_aceMesh_stack::pop(task*& t)
{
}

/*
concurrent_stack::concurrent_stack() 
{
}
concurrent_stack::~concurrent_stack()
{
}


void concurrent_stack::push(task* t)
{
    cs.lock();
    task_pool.push(t);
    cs.unlock();
}


bool concurrent_stack::try_pop(task*& t)
{
    t = NULL;
    if( cs.try_lock() )
    {
        if(task_pool.empty())
        {
            cs.unlock();
            return false;
        }    
        t = task_pool.top();
        task_pool.pop();
        cs.unlock();
        return true;
    }
    else 
        return false;
}
*/

/****************************************************
 * concurrent_aceMesh_queue
 * This data structure is not secure for concurrency.
 * This is used for extra_queue, which will be retained temporarily and modified to TBB in the future
 ****************************************************/
concurrent_aceMesh_queue::concurrent_aceMesh_queue() 
{
    head = tail = new node();
    //my_len = 0;
}
concurrent_aceMesh_queue::~concurrent_aceMesh_queue()
{
  delete head;
  head = tail = NULL;
}


void concurrent_aceMesh_queue::push(task* t)
{
    node* q = new node(t);
    q->next = NULL;
    node* p;
    do
    {
        p = tail;
    }while( p->next.compare_and_swap(q, NULL) != NULL );
    // __sync_bool_compare_and_swap(&p->next, NULL, q));
    tail.compare_and_swap(q, p);
    //__sync_bool_compare_and_swap(&tail, p , q);
    //++my_len;
}


bool concurrent_aceMesh_queue::try_pop(task*& t)
{
    node* p = head;
    if(p->next == NULL) 
    {
        return false;
    }
    if( head.compare_and_swap( p->next, p) == p)
    //if( __sync_bool_compare_and_swap(&head, p, p->next) )
    {
        t = p->next->t;
        //assert(t!=NULL);
        delete p;
        //--my_len;
        return true;
    }
    return false;
}


aceMesh_queue::aceMesh_queue() 
{
    head = tail = NULL;//new node();
}
aceMesh_queue::~aceMesh_queue()
{
    delete head;
    head = tail = NULL;
}


void aceMesh_queue::push(task* t)
{
    node* q = new node(t);
    q->next = NULL;
    if(head == NULL )
    {
         head=q;
         tail=q;
         return;
    }
    tail->next=q;
    tail=q;
}

bool aceMesh_queue::try_pop(task*& t)
{
    node* p = head;
    if(p == NULL) 
    {
        delete p;
        return false;
    }else{
        t=p->t;
        head=p->next;
        delete p;
        return true;
    }
    return false;
}

#ifdef MTEST_LIGHT

aceMesh_mtest_queue::aceMesh_mtest_queue() 
{
    head = tail = NULL;
}
aceMesh_mtest_queue::~aceMesh_mtest_queue()
{
    delete head;
    head = tail = NULL;
}

void aceMesh_mtest_queue::push(mtest_task* t)
{
    t->next = NULL;
    if( head == NULL )
    {
         head=t;
         tail=t;
         return;
    }
    tail->next=t;
    tail=t;
}

bool aceMesh_mtest_queue::try_pop(mtest_task*& t)
{
    mtest_task* old_top = head;
    if( old_top == NULL) 
    {
        return false;
    }else{
        t=old_top;
        head=old_top->next;
        return true;
    }
    return false;
}

/*void aceMesh_mtest_queue::push(mtest_task* t)
{
    node* q = new node(t);
    q->next = NULL;
    if( head == NULL )
    {
         head=q;
         tail=q;
         return;
    }
    tail->next=q;
    tail=q;
}

bool aceMesh_mtest_queue::try_pop(mtest_task*& t)
{
    node* p = head;
    if( p == NULL) 
    {
        delete p;
        return false;
    }else{
        t=p->t;
        head=p->next;
        delete p;
        return true;
    }
    return false;
}*/
#endif


}
