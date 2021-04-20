#ifndef _CONCURRENT_TASK_DAG_GRAPH_
#define _CONCURRENT_TASK_DAG_GRAPH_

#include "tbb/task_scheduler_init.h"
#include <tbb/task.h>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <string>
#include "tbb/concurrent_vector.h"
#include "tbb/concurrent_unordered_set.h"
#include "tbb/concurrent_unordered_map.h"
#include "tbb/mutex.h"

#include "concurrent_aceMesh_task.h"

namespace AceMesh_runtime {
//for interface input
typedef struct tuple_addr_type {
	void* addr;
    int area_type;
	int type;
	int neighbor;
	int neighbor_type;
}addr_tuple;

//for recording input for one virtual task
typedef struct tuple_addr_task {
	void* addr;
	int type;
	aceMesh_task* t;
    int significance;
}virtual_tuple;

//for hash map 
typedef struct task_addr_relation {
    aceMesh_task* t;
    int significance;//
    task_addr_relation():t(NULL), significance(0)
    {}
} task_significance;

//for hash_map
typedef struct tuple_rw_task {
	tbb::concurrent_vector<task_significance> r_tasks;
	task_addr_relation w_task;
	tuple_rw_task(){}
}rw_task_tuple;

//for check confict
struct update_addr_flag
{
    bool w_flag;
    bool r_flag;
    update_addr_flag(): w_flag(false), r_flag(false)
    {}
};


class concurrent_task_dag_graph {
public:
	concurrent_task_dag_graph();
	~concurrent_task_dag_graph();
	concurrent_task_dag_graph(int thread_nums);
    void init(int thread_num);
	
	Error_Code register_task(aceMesh_task* t, std::vector<addr_tuple>& addrs);
	Error_Code begin_split_task();
	Error_Code end_split_task();

	Error_Code end_construct_dag();
	Error_Code spawn();
	Error_Code wait();
	Error_Code spawn_and_wait();
#ifdef DEBUG_GRAPH
	Error_Code begin_split_task(const std::string& loop_info);
	Error_Code spawn_and_wait(int print_graph);
#endif

private:
    inline void* get_true_addr(void* addr, int type);
    int update_write_addr(std::map<void*,update_addr_flag>& addr_update, void* addr, int type);

    inline int build_releationship(task* dest, int type, tuple_rw_task& src, bool is_neighbor); 
    int thread_num;

	void reset_task_graph();
	//inline void do_analysis_addr();

	tbb::concurrent_vector<aceMesh_task*> need_spawn_tasks; 

	tbb::concurrent_unordered_map<void*, tuple_rw_task> addr_task;

    void insert_to_addr_hash(aceMesh_task* t, int type, void* addr, int significance,
         /* out */ std::pair<tbb::concurrent_unordered_map<void*, tuple_rw_task>::iterator, bool>& result);
    void update_to_addr_hash(aceMesh_task* t, int type, int significance, tuple_rw_task& second);

    //for merge addr and type
    inline int type_add(int type1, int type2);
    inline int significance_add(int significance1, int significace2);
    
	//need to judge whether task is end
    std::unordered_set<aceMesh_task*> end_tasks;
    tbb::mutex mutex_end_tasks;
    inline void add_end_task(aceMesh_task* t);
    inline void del_end_task(aceMesh_task* t);

    //TODO
    empty_task* join_task;

};
}
#endif
