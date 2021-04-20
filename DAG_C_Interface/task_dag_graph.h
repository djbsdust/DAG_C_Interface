#ifndef _TASK_DAG_GRAPH_
#define _TASK_DAG_GRAPH_
//#include "tbb/concurrent_unordered_map.h"
#include "aceMesh_runtime.h"
#include "tbb/task_scheduler_init.h"
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <string>
#include "pthread.h"

#include "dag_graph_check.h"
#include "affinity_id_generator.h"

#ifdef AUTO_PARTITION
#include "disjoin_set.h"
#endif

#ifdef ACEMESH_PERFORMANCE
#include "aceMesh_performance.h"
#endif

#ifdef DEBUG_GRAPH
#include "trace_out.h"
#endif



namespace AceMesh_runtime {
//ALERT!  TODO: DANGEROUS
//Is 32 neighbors enough for any application? we can use more
//it will not waste memory, since it is reused throughout the whole program.
#define MAX_NEIGHBORS 32
//for debug 
#ifdef DEBUG_GRAPH
class end_task : public aceMesh_task {
public:
    end_task(){
        aceMesh_task::set_joint();
    }
    virtual ~end_task(){}
#ifndef SWF		
    virtual void get_neighbor(int neighbor, void* src_addr, struct ptr_array *neighbor_addrs)
    {}
#else
    virtual void get_neighbor()
    {}

#endif
    //virtual task* execute();
    
};
#endif

//for mult DAG buffer
typedef struct dag_instance_type {
#ifdef DEBUG_GRAPH
    end_task* endTask;
#else 
	empty_task* endTask;
#endif
    std::vector<aceMesh_task*> need_spawn_tasks;
} dag_instance;

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
    std::vector<task_significance> r_tasks;
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


#ifdef ACEMESH_PERFORMANCE
#include "aceMesh_performance.h"
#endif



//seq 
class task_dag_graph {
private:
#ifdef _EXEC_TIME
    tbb::tick_count mainStartExecTime;
    tbb::tick_count mainEndExecTime;
#endif
#ifdef _DAG_TIME
    tbb::tick_count dagStartTime;
    tbb::tick_count dagEndTime;
#endif

int curr_dagNo;
public:
	task_dag_graph();
	~task_dag_graph();
	//task_dag_graph(int thread_nums);

    void init(int thread_num, const std::string& my_dir);
	Error_Code register_task(aceMesh_task* t, std::vector<addr_tuple>& addrs);
	Error_Code spawn_and_wait();
    void free_tasks();
#ifdef CONCURRENT_CONSTRUCT_GRAPH
	Error_Code wait_for_all_task();
#endif

#ifdef SPECIFY_END_TASKS
      void acemesh_add_end_task(aceMesh_task* t);
#endif

    Error_Code begin_split_task(const std::string& loop_info);
	Error_Code begin_split_task(); 
	Error_Code end_split_task();
    Error_Code dag_start(int dagNo, int *int_vec, int n1, double *float_vec, int n2);
    Error_Code reset_affinity();

#ifdef SWF 
   void push_neighbor_addr(void *addr) ;
#endif

    void init_thread_num(int thread_num);
    //for debug
#ifdef DEBUG_GRAPH
    Error_Code spawn_and_wait(int print_graph);
#endif

#ifdef SUPPORT_PARTITION
    void set_separations(std::map<void*, int>& sep_datas);
    Error_Code spawn_and_wait_with_separation();
#endif

#ifdef AUTO_PARTITION
    Error_Code spawn_and_wait_with_separation();
#endif

#ifdef DEBUG_GRAPH
	 inline int get_task_nums() {return task_nums;}
#endif


private:
    Error_Code spawn();
    Error_Code wait();
    Error_Code end_construct_dag();
    void _store_dag_info();
    void _save_to_buffer();
    void _resume_from_buffer();
#ifdef DEBUG_GRAPH
        bool check_graph();
    bool check_graph(int print_graph);
    bool g_print_graph;
#endif

#ifdef AUTO_PARTITION
    void spawn(int sep_id);
    //Error_Code spawn_and_wait_with_separation();
    void end_construct_dag_separation();
#endif

#ifdef SUPPORT_PARTITION
    void end_construct_dag_separation();
#endif

    inline void* get_true_addr(void* addr, int type);
    int update_write_addr(std::map<void*,update_addr_flag>& addr_update, void* addr, int type);
    inline int build_releationship(aceMesh_task* dest, int type, tuple_rw_task& src, bool is_neighbor); 
    int thread_num;
    void add_tuples_to_virtual_task(aceMesh_task* t, void* addr, int type, int significance);
	void reset_task_graph();
	inline void do_analysis_addr();

	std::vector<aceMesh_task*> need_spawn_tasks;
    std::unordered_map<void*, tuple_rw_task> addr_task;

    std::vector<tuple_addr_task> one_virtual_task_tuples;
    void unique_tuples();
    inline int type_add(int type1, int type2);
    inline int significance_add(int significance1, int significace2);
    
	//need to judge whether task is end
	std::unordered_set<aceMesh_task*> end_tasks;
    inline void add_end_task(aceMesh_task* t);
    inline void del_end_task(aceMesh_task* t);

    //reuse
    std::unordered_map<int, std::vector<double>> dag_fvec_map;
    std::unordered_map<int, std::vector<int>> dag_ivec_map;
    std::unordered_map<int, int> dag_reuse_cnt;
    std::unordered_map<int, bool> dag_reuse_flag;
	std::unordered_set<task*> wait_free_tasks;
    //dag buffer
    std::unordered_map<int, dag_instance> dag_instance_map;
#ifndef SWF    
    struct ptr_array neighbor_addrs;
#else
	void* neighbor_addrs[MAX_NEIGHBORS];    
	int num_neighbs;
#endif	
	
#ifdef DEBUG_GRAPH
    end_task* endTask;
#else 
	empty_task* endTask;
#endif

    //for debug
#ifdef DEBUG_GRAPH
    dag_graph_check graph_check;
    bool check_graph_dfs();
    bool check_graph_bfs();
    int task_nums;
#endif

#ifdef AUTO_AFFINITY
    affinity_id_generator my_affinity;
#endif

#ifdef AUTO_PARTITION
    int sep_id;
    disjoin_set my_disjoin_set;
    std::map<int,aceMesh_task*> sep_task;
    std::map<int, std::vector<int> > sep_data;
    std::map<int, task*> sep_end_tasks;
    void union_set(aceMesh_task* src, aceMesh_task* dest);
#endif

#ifdef SUPPORT_PARTITION
    //std::map<void*, int> addr_sep_datas; 
    std::map<int, std::vector<aceMesh_task*> > sep_data;
    std::map<int, task*> sep_end_tasks;
#endif

};
}
#endif
