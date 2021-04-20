#ifndef _ACE_MESH_TASK_
#define _ACE_MESH_TASK_

#include <vector>
#include <string>
#include <unordered_set>

#if defined(DYNAMIC_SCHEDULER)
#include <tbb/task.h>
using namespace tbb;
#elif defined(ACEMESH_SCHEDULER)
#include "task.h"
#endif

#include <tbb/tick_count.h>
#include "tbb/atomic.h"
#include "tbb/enumerable_thread_specific.h"

#include <iostream>

namespace AceMesh_runtime {
//TODO
//for task type
typedef int task_type;
#define NOT_SET 0
#define STENCIL_TASK    1
#define NOAFFINITY_TASK 2
#define BLOCKING_TASK 3   //for mpi_wait, or mpi_send,mpi_recv etc blocking ops.
//end

#ifdef SUPPORT_PARTITION
#define NO_SET_PART_ID -1
#endif

#ifdef ACEMESH_TIME
typedef tbb::enumerable_thread_specific< std::pair<double, double> > aceMesh_double_TLS;
typedef tbb::enumerable_thread_specific< double> aceMesh_double1_TLS;
#endif
#ifdef ACEMESH_SCHEDULER_PROFILING
typedef tbb::enumerable_thread_specific< double > aceMesh_long_TLS;
#endif

class aceMesh_task: public task {
#ifdef SAVE_RW_INFO
private:
    struct addr_info
    {
        void* addr;//
        int area_type;
        int rw_type;//r, w, rw,etc
        bool is_neighbor;//
        addr_info(void* addr, int area_type, int rw_type, bool is_neighbor): 
            addr(addr), area_type(area_type), rw_type(rw_type), is_neighbor(is_neighbor)
        {}
    };
    std::vector<addr_info> rw_addrs;
public:
    void add_addr(void* addr, int area_type, int rw_type, bool is_neighbor)
    {
        rw_addrs.push_back(addr_info(addr, area_type, rw_type, is_neighbor));
    }
	 std::vector<addr_info>& get_rw_addrs()  {return rw_addrs;}

#endif
#ifndef NOT_MPI_STRATEGY
  //lchen, for blocking ops
  private:
          int suspend;				
  public:
          inline void set_suspend(int mode){suspend=mode;}
          inline int get_suspend(){return suspend;}   
  //end of blocking ops
#endif	

#ifdef SUPPORT_PARTITION
private:
    int my_part_id; 
public:
    inline void set_part_id(int id){my_part_id = id;}
    inline int get_part_id(){ return my_part_id;}
#endif

#ifdef AUTO_PARTITION
private:
    int group_id;
public:
    inline void set_group_id(int id){group_id = id;}
    inline int get_group_id(){ return group_id;}
#endif

#ifdef ACEMESH_TIME
private:
    tbb::tick_count start_t;
protected:
    virtual void record_execute_time();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
private:
    long long perf_cnt;
protected:
    virtual void record_papi_perf_cnt();
#endif

private:
#ifdef ACEMESH_SCHEDULER_PROFILING
    static tbb::atomic<long long> sum_vert_times;
    static tbb::atomic<int> max_reuse_distance;
    int reuse_distance;
#endif

#ifdef DEBUG_GRAPH
    int is_joint;
    int ref_count_for_check;
    int loop_id;
    int task_id;
    //TODO
    //std::vector<addr*> wliat;
    //std::vector<addr*> rlist;
#endif

#ifdef ACEMESH_TIME
private:
    static aceMesh_double_TLS time_tls;
    static aceMesh_double1_TLS time_blocking;
public :
    static void print_and_reset_execute_time();
#endif

#ifdef ACEMESH_SCHEDULER_PROFILING
private:
    static aceMesh_long_TLS thread_total_task_num;
    void inc_reuse_distance(int);
    int get_reuse_distance();
public :
    static void print_and_reset_reuse_statistics();
    static long long total_vert_times;	 
    static tbb::atomic<long long> stolen_times;
#endif
#ifdef _EXEC_TIME
public:
	static double exec_time;
	static tbb::atomic<long long> total_task_num;
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
public:
	static double cache_miss;
#endif

#ifdef DEBUG_GRAPH
public :
    void set_loop_id(int id);
    void set_task_id(int id);
    int  get_loop_id();
    int  get_task_id();
    void set_joint();
    int get_ref_count_for_check();
    void set_ref_count_for_check(int value);
    int dec_ref_count_for_check();
    void dfs(int& task_nums, int deep_length);
    aceMesh_task* get_vertical_task();
    aceMesh_task* get_successor_task(int i);
    bool is_end_task();
    int  get_successor_sum();
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
    tbb::mutex mutex_add_successor;
#endif

private:
	std::vector<task* > successor_tasks;
	task* vertical_task;
    //core
    task_type my_type;

public:
	aceMesh_task();
    aceMesh_task(int id);
    virtual ~aceMesh_task();
    inline void set_task_type(task_type type)
    {
        my_type = type;
    }
    inline task_type get_task_type()
    {
        return my_type;
    }
	/********************************************************************************************/
    /*actually,the first two args, neighbor and src_addr, are defuncted by the C-user interface  */
	/*users use new packded args, please refer to the definition of NEIGHBOR_FUNCPTR             */
	/*in aceMesh_runtime_c.h                                                                     */
	/*********************************************************************************************/
#ifndef SWF
    virtual void get_neighbor(int neighbor, void* src_addr, struct ptr_array *neighbor_addrs);
#else
    virtual void get_neighbor( );
#endif

    int  get_total_successor_sum();
    void store_info();
    void get_free_list(std::unordered_set<task *> &wait_free_tasks);
    void add_successor(aceMesh_task *t);
    void set_vertical_task(aceMesh_task* t);
    //for add join task
    //NEW 2014-1-25
    void add_end_successor(task* t);

	virtual task* execute();

#if defined(DYNAMIC_SCHEDULER)
	void set_affinity_id(int id){}
#endif

    //TODO
#ifdef PAPI_PERFORMANCE
protected:
	void papi_performance_start();
	void papi_performance_end();
#endif

#if defined(ACEMESH_SCHEDULER_PROFILING_EACH_TASK)  || defined(DEBUG_GRAPH)
public:
  static std::vector<std::string> loop_names;
#endif
#ifdef  DEBUG_GRAPH
public:
  static	 int get_same_addr_count(aceMesh_task* t_parent, aceMesh_task* t_child) {
      std::vector<addr_info> &rw_addrs_p= t_parent->get_rw_addrs();
      std::vector<addr_info> &rw_addrs_c= t_child->get_rw_addrs();

	   int count = 0;
      for(std::vector<addr_info>::iterator itr_p = rw_addrs_p.begin(); itr_p != rw_addrs_p.end(); ++itr_p){
        void *addr_p = itr_p -> addr;
		 for(std::vector<addr_info>::iterator itr_c = rw_addrs_c.begin(); itr_c != rw_addrs_c.end(); ++itr_c) {	
		   void *addr_c = itr_c -> addr;
			if(addr_p == addr_c) {
			  count++;
			}
		 }
     }
    return count;
}

#endif

};
}
#endif
