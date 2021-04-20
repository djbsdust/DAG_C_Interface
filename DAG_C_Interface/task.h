#ifndef _TASK_H
#define _TASK_H
 
#include "tbb/atomic.h"
#include "tbb/mutex.h"
#include <vector>

#include "aceMesh_runtime_c.h"
namespace AceMesh_runtime {
#define NO_SET_AFFINITY -1
#define FOLLOW_AFFINITY -2
#define INIT_AFFINITY   0   //-3

//TODO
enum task_state
{
    ALLOCATED=0,
    READY,
    RUNNING,
    FREED
};

class task 
{
    private:
        tbb::atomic<int> ref_count_t;
        int backup_ref;
        bool reused;//if marked, when executing dont delete
        bool stored;//avoid redundant traversal when store ref
#ifdef CONCURRENT_CONSTRUCT_GRAPH
        tbb::atomic<int> pre;
#endif
        int affinity_id;
        int priority_id;
        int spawn_order_id;
#ifdef NUMA_PROFILING
	std::vector<void *> addr_vector;
#endif
    protected:
        void adjust_affinity_id(task* another_task);

    public:
        task* next;
		task();
		virtual ~task();
		task(int id);
        virtual task* execute() = 0;
        void increment_ref_count();
        int decrement_ref_count();
        void wait_for_all();
        int ref_count() const;
        void store_ref_count();
        void restore_ref_count();
        void set_stored(bool val);
        bool get_stored() const;
        void set_reused_flag(bool val);
        bool get_reused_flag();
        int get_affinity_id() const;
        void set_affinity_id(int id);

        int get_priority_id() const; 
        void set_priority_id(int id);

        int get_spawn_order_id() const;
        void set_spawn_order_id(int id);

        static void spawn(task& t);
        static void enqueue(task& t);
        static void spawn( std::vector<task*>& init_task_list );
        static void init_spawn( task& t );
#ifdef CONCURRENT_CONSTRUCT_GRAPH
        void set_pre(int p);
        int pre_count() const;
#endif

#ifndef NOT_MPI_STRATEGY				
  #ifdef EXTRA_THREAD //
        static void mpi_spawn(task& t);
#ifdef MTEST_LIGHT
        static void mpi_polling_spawn(task& t);
#endif
  #endif
  #ifdef BLOCKING_QUEUE
        static void suspend_spawn(task& t);
  #endif
#endif
#ifdef ACEMESH_TIME
        virtual void record_execute_time() = 0;
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
    virtual void record_papi_perf_cnt() = 0;
#endif
        /** 0x0 -> version 1.0 task
         0x1 -> version >=2.1 task
         0x10 -> task was enqueued 
         0x20 -> task_proxy
         0x40 -> task has live ref_count
         0x80 -> a stolen task */
        unsigned char extra_state;
        bool is_stolen_task() const
        {
            return extra_state & 0x80;
        }
#ifdef NUMA_PROFILING
	void set_addr_vector(std::vector<void*>& addr_vec);
	std::vector<void*> get_addr_vector();
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
public:
        tbb::mutex finished_lock; 
        tbb::atomic<bool> edge;
        tbb::atomic<bool> ready;
        tbb::atomic<bool> finish;
        tbb::atomic<bool> over;
#endif

};

#ifdef MTEST_LIGHT
class mtest_task
{
    public:
      int * comm_handle1;
      int * comm_handle2;
      task* comm_task;
      int comm_kind1;
      int comm_kind2;
    public:
      mtest_task* next;
	  //mtest_task();
	  //mtest_task(int* id1,int* id2,task* t);
	  //virtual ~metst_task();
//      void mtest_task::set_comm_handle1(int id);
//      void mtest_task::set_comm_handle2(int id);
//      void mtest_task::set_comm_task(task* id);
};

#endif


#ifdef MTEST_LIGHT2
inline void mtest_task::set_comm_handle1(int id)
{
    comm_handle1 = id;
}
inline void mtest_task::set_comm_handle2(int id)
{
    comm_handle2 = id;
}
inline void mtest_task::set_comm_task(task* id)
{
    comm_task = id;
}
#endif
inline void task::increment_ref_count()
{
	++ref_count_t;
}
inline int task::decrement_ref_count()
{
    return --ref_count_t;
}
inline int task::ref_count() const
{
    return ref_count_t;
}

inline void task::store_ref_count()
{
    backup_ref =  ref_count_t;
}
inline void task::restore_ref_count()
{
    ref_count_t = backup_ref;
}
inline void task::set_stored(bool flag)
{
    stored = flag;
}
inline bool task::get_stored() const
{
    return stored;
}
inline void task::set_reused_flag(bool val)
{
    reused = val;
}
inline bool task::get_reused_flag()
{
    return reused;
}
inline int task::get_affinity_id() const
{
    return affinity_id;
}

inline void task::set_affinity_id(int id)
{
    affinity_id = id;
}

inline int task::get_priority_id() const
{
    return priority_id;
}
inline void task::set_priority_id(int id)
{
    priority_id = id;
}

inline int task::get_spawn_order_id() const
{
    return spawn_order_id;
}
inline void task::set_spawn_order_id(int id)
{
    spawn_order_id = id;
}
#ifdef CONCURRENT_CONSTRUCT_GRAPH
inline void task::set_pre(int p)
{
      pre=p;
}
inline int task::pre_count() const
{
    return pre;
}
#endif

#ifdef NUMA_PROFILING
inline void task::set_addr_vector(std::vector<void*>& addr_vec){
    addr_vector.assign(addr_vec.begin(),addr_vec.end());
}
inline std::vector<void*> task::get_addr_vector(){
    return addr_vector;
}
#endif
//empty_task
class empty_task : public  task
{
public:
    virtual task* execute(){ return NULL;}
#ifdef ACEMESH_TIME
     virtual void record_execute_time(){};
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
    virtual void record_papi_perf_cnt(){};
#endif

};
}


#endif
