#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_priority_queue.h"
#include "tbb/atomic.h"
#include "task.h"
#include <vector>
#include <iostream>
#include <pthread.h>
#include "aceMesh_stack.h"
#include "aceMesh_utils.h"

#include "aceMesh_deque.h"
#include "aceMesh_scheduler_init_v2.h"


#include "scheduler_common.h"

namespace AceMesh_runtime {


typedef void (*tls_dtor_t)(void*);
template <typename T>
class basic_tls {
    typedef pthread_key_t tls_key_t;
public:
    int  create( tls_dtor_t dtor = NULL ) {
        return pthread_key_create(&my_key, dtor);
    }
    int  destroy()      { return pthread_key_delete(my_key); }
    void set( T value ) { pthread_setspecific(my_key, (void*)value); }
    T    get()          { return (T)pthread_getspecific(my_key); }
private:
    tls_key_t my_key;
};

void auto_do_exit(void* p);


class generic_scheduler
{
public:
	static int num_threads;
    static basic_tls<generic_scheduler*> theTLS;
    static std::vector<generic_scheduler*> schedulers;
    static std::vector<pthread_t> threads;
		
#ifdef EXTRA_THREAD //
    static generic_scheduler* mpi_scheduler;
    static pthread_t mpi_thread;

#ifdef USE_TBB_QUEUE_MPI
    static tbb::concurrent_queue<task*> mpi_queue;
#else
    static concurrent_aceMesh_queue  mpi_queue;
#endif

#ifdef MTEST_LIGHT
    //static tbb::concurrent_queue<mtest_task*> polling_queue;
    static aceMesh_mtest_queue polling_queue;
#endif

#endif
#ifdef BLOCKING_QUEUE
      aceMesh_queue  blocking_queue;   
#endif
#ifdef USE_SHARED_QUEUE 
	static tbb::concurrent_queue<task*> shared_queue;
#endif
    
#ifdef USE_SHARED_STACK
    static concurrent_aceMesh_stack shared_queue;
#endif

#ifdef	USE_STEALING_SHARED
	static concurrent_aceMesh_stack shared_queue[NUMA_num];
#endif
	//static tbb::concurrent_bounded_queue<task*> shared_queue;
    //
private:
    static tbb::atomic<int> index;
    int affinity_cpu_id;
    int id;//scheduler
    //tbb::atomic<bool> close;

    int stolen_tol_times ;

//private queue
#ifdef USE_DEQUE
    //FastRandom my_random;
    aceMesh_deque private_queue;

#ifdef USE_MY_OPT_STRATEGY 
    //tbb::concurrent_queue<task*> my_data_queue;
    concurrent_aceMesh_queue my_data_queue;
#endif

#endif

#ifdef USE_TBB_QUEUE
    tbb::concurrent_queue<task*> private_queue;
#endif

#ifdef USE_STACK
    concurrent_aceMesh_stack private_queue;
#endif

#ifdef USE_PRIORITY_QUEUE
    class compare_task
    {
        public:
            bool operator()(const task* u, const task* v) const 
            {
				//actually, tbb regards large priority_value as high
				//but acemesh NOT!
                return u->get_priority_id() > v->get_priority_id();
            }
    };
    tbb::concurrent_priority_queue<task*, compare_task> private_queue;
#endif

#ifdef USE_STEALING
    FastRandom my_random;
#endif

#ifdef USE_STEALING_NUMA
    FastRandom my_random;
#endif

    
public:
    generic_scheduler();
    generic_scheduler(int cpu_id);
    ~generic_scheduler();

	void local_spawn(task* t);
	void init_spawn(task* t);
    void enqueue(task* t);
	void enqueue(task *t,int task_id);
#ifndef NOT_MPI_STRATEGY
  #ifdef EXTRA_THREAD
	  void mpi_local_spawn(task* t);
#ifdef MTEST_LIGHT
      void mpi_polling_local_spawn(task* t);
#endif
  #endif
  #ifdef BLOCKING_QUEUE  //add  wangm
    void mpiqueue(task* t);
  #endif
#endif
    task* get_next_task();
#ifdef MTEST_LIGHT
	mtest_task* get_next_mtest_task();
#endif

    void spawn_to_id(task* t); 

    inline int get_myid(){ return id;}    
#ifdef USE_STEALING
    task* stolen_task();
#endif 
#ifdef USE_STEALING_NUMA
    task* stolen_task();
#endif 
	//USE_MY_OPT_STRATEGY can be used when USE_STEALING is off
	//previous Hou's codes said the opposite.
#ifdef USE_MY_OPT_STRATEGY 
    task* copy_data();
    void remote_spawn(task* t);
#endif

//#endif //wrong code of Hou xionghui, deprecated by chen, li

    //for all scheduler
    static void wait_for_all();
    static pthread_t new_thread(void* closure);
    static void create_worker_thread(int n_threads);
    static void create_worker_thread(int n_threads, int p[]);
    static void close_worker_thread();
    static void* worker_kernel_func(void* p); 
    static void main_kernel_func(task* p); 
#ifdef EXTRA_THREAD //WANGM
    static void* mpi_worker_kernel_func(void* p); 
    static pthread_t new_mpi_thread(void* closure);
#endif

#ifdef __ACEMESH_THREAD_GROUP
    aceMesh_observer* ob;
    static void create_worker_thread(aceMesh_observer* ob, int n_threads, int p[]);
#endif
    
};
}

#endif
