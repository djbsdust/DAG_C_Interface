#include "task_dag_graph.h"
#include <iostream>
#include <cassert>
#include <map>
#include <algorithm>
#include "tbb/tick_count.h"
#include "addr_area_manager.h"
#include <algorithm>

extern int sche_num_threads;
extern int my_mpi_rank;
namespace AceMesh_runtime {
//int time_for_v_tasks = 0;
#ifdef _EXEC_TIME
double exec_time = 0.0;
#endif
#ifdef _DAG_TIME
double dag_build_time = 0.0;
double dag_reuse_time = 0.0;
#endif

//bool task_dag_graph::isInit = false;

#ifdef AUTO_PARTITION
task_dag_graph::task_dag_graph():my_disjoin_set(10240)
{
#ifdef DEBUG_GRAPH
    task_nums = 0;
    g_print_graph = false;
#endif
    sep_id = 0;
}
#else
task_dag_graph::task_dag_graph()
{
#ifdef DEBUG_GRAPH
    task_nums = 0;
    g_print_graph = false;
#endif
    curr_dagNo = 0;
}
#endif


task_dag_graph::~task_dag_graph()
{
/*
#ifdef _EXEC_TIME
    std::cout<< "exec time : " << exec_time << std::endl;
#endif

#ifdef ACEMESH_SCHEDULER_PROFILING
    //aceMesh_task::print_and_reset_execute_time();
    std::cout << "total vert times : " << aceMesh_task::total_vert_times << std::endl;
    std::cout << "total stolen times : " << aceMesh_task::stolen_times << std::endl;
#endif
*/
}

void task_dag_graph::init(int thread_num, const std::string& out_dir)
{
#ifdef AUTO_AFFINITY
    my_affinity.set_thread_num(thread_num);
#endif

#ifdef DEBUG_GRAPH
	graph_check.init(out_dir);
#endif
}

#ifdef AUTO_PARTITION
inline void task_dag_graph::union_set(aceMesh_task* src, aceMesh_task* dest)
{
    if(dest->get_group_id() < 0) 
    {
        dest->set_group_id( src->get_group_id() );
    }
    else
        dest->set_group_id( my_disjoin_set.union_set(src->get_group_id(), dest->get_group_id()) );
}
#endif
Error_Code task_dag_graph::dag_start(int dagNo, int *int_vec, int n1, double *float_vec, int n2) {
#ifdef _DAG_TIME
    if ((dagStartTime - tbb::tick_count()).seconds() == 0)
        dagStartTime = tbb::tick_count::now();
#endif
    curr_dagNo = dagNo;
    dag_reuse_flag[dagNo] = false;
    auto iter = dag_reuse_cnt.find(dagNo);
    if (iter != dag_reuse_cnt.end()) {
        auto vecI = dag_ivec_map[dagNo];
        auto vecF = dag_fvec_map[dagNo];
        //compare vec length
        if (vecI.size() != n1 || vecF.size() != n2) {
            return 0;
        }

        for (int i = 0; i < n1; ++i) {
            if (vecI[i]!=int_vec[i])
                return 0;
        }
        for (int i = 0; i < n2; ++i) {
            if (vecF[i]!=float_vec[i])
                return 0;
        }
        std::cout << "reuse: "<< dagNo << std::endl;
        iter->second++;
        dag_reuse_flag[dagNo] = true;
        return 1;
    } else {
        //init
        dag_reuse_cnt[dagNo] = -1;
        for (int i = 0; i < n1; ++i) {
            dag_ivec_map[dagNo].push_back(int_vec[i]);
        }
        for (int i = 0; i < n2; ++i) {
            dag_fvec_map[dagNo].push_back(float_vec[i]);
        }
        return 0;
    }
}

inline int task_dag_graph::build_releationship
    (aceMesh_task* dest, int type, tuple_rw_task& src, bool is_neighbor) 
{
#ifdef CONCURRENT_CONSTRUCT_GRAPH
    int ed1,ed2;//carry return val of add or delete;
#endif    
    int res = 0;
	aceMesh_task* tmp = NULL;
	if(src.r_tasks.size() == 0)
	{
		if(src.w_task.t != NULL)
		{
#ifdef CONCURRENT_CONSTRUCT_GRAPH
        if(src.w_task.t->finish == false)
        {
#endif
			tmp = src.w_task.t;
            if( !is_neighbor && src.w_task.significance == 0)
            {
                if(type != INOUT_NONE)
                {
#ifndef SPECIFY_END_TASKS
                    del_end_task(tmp);
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
                    ed1 = tmp->set_vertical_task(dest);
                    res += ed1;
                    if (src.w_task.t->finish == true)
                    {
                        if (dest->edge)
                        {
                            std::cout << "finished1 :" << src.w_task.t->finish << std::endl;
                            ed2 = src.w_task.t->del_successor(dest);
                            res -= ed2;
                        }
                    }
                    dest->edge = false;
#else
                    tmp->set_vertical_task(dest);
                    ++res;
#endif
                
#ifdef AUTO_PARTITION
                    union_set(tmp, dest);
#endif
                }
            }
            else if(is_neighbor || src.w_task.significance == 1)
            {
                if(type != INOUT_NONE)
                {
#ifndef SPECIFY_END_TASKS
              del_end_task(tmp);
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
			        ed1=tmp->add_successor(dest);
                    res += ed1;
                    if (src.w_task.t->finish == true)
                    {
                        if (dest->edge)
                        {
                            std::cout << "res " << res << std::endl;
                            std::cout << "finished2 :" << src.w_task.t->finish << std::endl;
                            ed2 = src.w_task.t->del_successor(dest);
                            res -= ed2;
                            std::cout << "res " << res << std::endl;
                        }
                    }
                    dest->edge = false;
#else
			        tmp->add_successor(dest);
                    ++res;
#endif

#ifdef AUTO_PARTITION
                    union_set(tmp, dest);
#endif
                }
            }
            else 
            {
                assert(0);
            }
            //std::cout<<"add edge\n";
#ifdef CONCURRENT_CONSTRUCT_GRAPH
        }//pair with if(finish) 
#endif
		}
		else
        {
            assert(0);
            //std::cout<<"1"<<std::endl;
        }
	}
	else 
	{
		if(type == IN)
		{
			if(src.w_task.t != NULL)
			{
#ifdef CONCURRENT_CONSTRUCT_GRAPH
            if(src.w_task.t->finish == false)
            {
#endif
				tmp = src.w_task.t;
                if(!is_neighbor && src.w_task.significance == 0)
                {
                    if(type != INOUT_NONE)
                    {
#ifndef SPECIFY_END_TASKS
                        del_end_task(tmp);
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
                        ed1 = tmp->set_vertical_task(dest);
                        res += ed1;
                        if (src.w_task.t->finish == true)
                        {
                            if (dest->edge)
                            {
                                std::cout << "finished3 :" << src.w_task.t->finish << std::endl;
                                ed2 = src.w_task.t->del_successor(dest);
                                res -= ed2;
                            }
                        }
                        dest->edge = false;
#else
                        tmp->set_vertical_task(dest);
                        ++res;
#endif

#ifdef AUTO_PARTITION
                    union_set(tmp, dest);
#endif
                    }
                }
                else if( is_neighbor || src.w_task.significance == 1)
                {
                    if(type != INOUT_NONE)
                    {
#ifndef SPECIFY_END_TASKS
                        del_end_task(tmp);
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
                        ed1 = tmp->add_successor(dest);
                        res += ed1;
                        if (src.w_task.t->finish == true)
                        {
                            if (dest->edge)
                            {
                                std::cout << "finished4 :" << src.w_task.t->finish << std::endl;
                                ed2 = src.w_task.t->del_successor(dest);
                                res -= ed2;
                            }
                        }
                        dest->edge = false;
#else
                        tmp->add_successor(dest);
                        ++res;
#endif

#ifdef AUTO_PARTITION
                    union_set(tmp, dest);
#endif
                    }
                }
                else 
                {
                    assert(0);
                }
                //std::cout<<"add edge\n";
#ifdef CONCURRENT_CONSTRUCT_GRAPH
            }//pair with if(finish) 
#endif
		    }
			else
            {
                //don't have depence task
                return 0;
            }
		}
		else 
		{
			//tmp = search_itr->second.r_tasks;
            for(std::vector<task_significance>::iterator r_task_itr = src.r_tasks.begin(); 
                    r_task_itr != src.r_tasks.end(); ++r_task_itr)
            {
#ifdef CONCURRENT_CONSTRUCT_GRAPH
            if(src.w_task.t->finish == false)
            {
#endif
                if(!is_neighbor && (*r_task_itr).significance == 0)
                {
                    if(type != INOUT_NONE)
                    {
#ifndef SPECIFY_END_TASKS
                        del_end_task(r_task_itr->t);
#endif
#ifdef CONCURRENT_CONSTRUCT_GRAPH
                        ed1 = r_task_itr->t->set_vertical_task(dest);
                        res += ed1;
                        if (r_task_itr->t->finish == true)
                        {
                            if (dest->edge)
                            {
                                std::cout << "res1 :" << res << std::endl;
                                std::cout << "finished5 :" << r_task_itr->t->finish << std::endl;
                                ed2 = r_task_itr->t->del_successor(dest);
                                res -= ed2;
                                std::cout << "res2 :" << res << std::endl;
                            }
                        }
                        dest->edge = false;
#else
                        r_task_itr->t->set_vertical_task(dest);
                        ++res;
#endif

#ifdef AUTO_PARTITION
                    union_set(r_task_itr->t, dest);
#endif
                    }
                }
                else if( is_neighbor || (*r_task_itr).significance == 1)
                {
                    if(type != INOUT_NONE)
                    {
#ifndef SPECIFY_END_TASKS
                        del_end_task(r_task_itr->t);
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
                        ed1 = r_task_itr->t->add_successor(dest);
                        res += ed1;
                        if (r_task_itr->t->finish == true)
                        {
                            if (dest->edge)
                            {
                                std::cout << "res1 :" << res << std::endl;
                                std::cout << "finished6 :" << r_task_itr->t->finish << std::endl;
                                ed2 = r_task_itr->t->del_successor(dest);
                                res -= ed2;
                                std::cout << "res2 :" << res << std::endl;
                            }
                        }
                        dest->edge = false;
#else
                        r_task_itr->t->add_successor(dest);
                        ++res;
#endif
#ifdef AUTO_PARTITION
                    union_set(r_task_itr->t, dest);
#endif
                    }
                }
                else 
                {
                    assert(0);
                }
			    //(*r_task_itr)->add_successor(t);
                //std::cout<<"add edge\n";
#ifdef CONCURRENT_CONSTRUCT_GRAPH
            }//pair with if(finish) 
#endif
            }
		}
    }
    return res;
}

inline void* task_dag_graph::get_true_addr(void* addr, int type)
{
    void* true_addr = NULL;
    if(type == NORMAL)
    {
        true_addr = addr;
    } 
    else if(type == SHADE) 
    {
        true_addr = get_shade_addr(addr);
    }
    else if(type == UNSHADE)
    {
        true_addr = get_unshade_addr(addr);
    }
    else 
    {
        assert(0);
    }
    return true_addr;
}


#ifdef SPECIFY_END_TASKS
void task_dag_graph::acemesh_add_end_task(aceMesh_task* t)
{
    end_tasks.insert(t);
}
#endif


Error_Code task_dag_graph::register_task(aceMesh_task* t, std::vector<addr_tuple >& addrs)
{
#ifdef CONCURRENT_CONSTRUCT_GRAPH
    t->ready = false;
    t->finish = false;
    t->over = false;
#endif

#ifndef SPECIFY_END_TASKS
      add_end_task(t);
#endif


#ifdef AUTO_AFFINITY
    t->set_affinity_id(my_affinity.get_affinity_id());
#endif
#ifdef DEBUG_GRAPH
    task_nums++;
    graph_check.set_task_loop_id(t);
#endif

#ifdef _EXEC_TIME
    AceMesh_runtime::aceMesh_task::total_task_num++;
#endif    

//#ifdef SUPPORT_PARTITION
//    std::map<void*,int>::iterator part_id_itr;
//#endif

	int sum_pre = 0;
    //time_for_v_tasks = 0;
#ifndef SWF
  neighbor_addrs.arr= new void* [MAX_NEIGHBORS];
#endif

	for(std::vector<addr_tuple>::iterator itr = addrs.begin(); itr != addrs.end(); ++itr)
	{
        if (itr->addr != NULL)
        {
//#ifdef SUPPORT_PARTITION
//            part_id_itr = addr_sep_datas.find(itr->addr);
//            if( part_id_itr == addr_sep_datas.end() )
//            {
//                assert(0);
//            }
//            t->set_part_id(part_id_itr->second);
//#endif
            //get true inout addr
            void* true_addr = this->get_true_addr(itr->addr, itr->area_type);
            //record this inout addr 
            this->add_tuples_to_virtual_task(t , true_addr, itr->type, 0);
            #ifdef SAVE_RW_INFO
            t->add_addr(itr->addr, itr->area_type, itr->type, 0);
            #endif

            //TODO: add shade and unshade area's  depend  to narmal  
		    //std::unordered_map<void*, tuple_rw_task>::iterator search_itr = addr_task.find(itr->addr);
	        //if(search_itr == addr_task.end())
	        //{ 
		    //    continue;
	        //}
		
            std::unordered_map<void*, tuple_rw_task>::iterator search_itr = addr_task.find(true_addr);
	        if(search_itr != addr_task.end())
	        { 
                sum_pre += build_releationship(t, itr->type, search_itr->second, false);
	        }
        }
	    if( itr->neighbor == NEIGHBOR_NONE )
            continue;
#ifndef SWF      
      neighbor_addrs.len=0;
      //note: this para is itr->addr
      t->get_neighbor(itr->neighbor, itr->addr, &neighbor_addrs);
#else
      num_neighbs=0;
      t->get_neighbor();
#endif        
        //get neighbors tasks and build releationship
#ifndef SWF         
      for(int i=0; i<neighbor_addrs.len; i++) {
        //get true addr
        void* neighbor_true_addr = this->get_true_addr(neighbor_addrs.arr[i], itr->area_type);
        //record,later read/write to hash_map
        this->add_tuples_to_virtual_task(t , neighbor_true_addr, itr->neighbor_type, 1);
        #ifdef SAVE_RW_INFO
        t->add_addr(neighbor_addrs.arr[i], itr->area_type, itr->neighbor_type, 1);
        #endif
        //std::cout<<"get neggggg:::"<<*neighbor_itr<<std::endl;
        std::unordered_map<void*, tuple_rw_task>::iterator search_itr = 
            addr_task.find(neighbor_true_addr);
        if(search_itr == addr_task.end()) { 
        //std::cout<<"find none"<<std::endl;
          continue;
        }
        sum_pre += build_releationship(t, itr->neighbor_type, search_itr->second, true);
      }
#else
      for(int i=0; i<num_neighbs; i++) {
        //get true addr
        void* neighbor_true_addr = this->get_true_addr(neighbor_addrs[i], itr->area_type);
        //record,later read/write to hash_map
        this->add_tuples_to_virtual_task(t , neighbor_true_addr, itr->neighbor_type, 1);
        #ifdef SAVE_RW_INFO
        t->add_addr(neighbor_addrs[i], itr->area_type, itr->neighbor_type, 1);
        #endif
        //std::cout<<"get neggggg:::"<<*neighbor_itr<<std::endl;
        std::unordered_map<void*, tuple_rw_task>::iterator search_itr = 
            addr_task.find(neighbor_true_addr);
        if(search_itr == addr_task.end()) { 
        //std::cout<<"find none"<<std::endl;
          continue;
        }
        sum_pre += build_releationship(t, itr->neighbor_type, search_itr->second, true);
      }
#endif      
    }
  #ifndef SWF
    delete []neighbor_addrs.arr;
  #endif

  if (sum_pre == 0) {
    need_spawn_tasks.push_back(t);
#ifdef AUTO_PARTITION
    my_disjoin_set.make_set(sep_id);
    sep_task[sep_id] = t;
    t->set_group_id(sep_id++);
#endif

#ifdef SUPPORT_PARTITION
    int id = t->get_part_id();
    std::map<int, std::vector<aceMesh_task*> >::iterator itr = sep_data.find( id );
    if(itr == sep_data.end() )
    {
        std::vector<aceMesh_task*> tmp;
        tmp.push_back(t);
        sep_data.insert( std::pair<int,std::vector<aceMesh_task*> >(id, tmp) );
    }
    else 
    {
        itr->second.push_back(t);
    }
#endif
	}
//#ifdef AUTO_AFFINITY
//    else
//    {
//        t->set_affinity_id(-2);
//    }
//#endif
    //std::cout << "v_task : " << time_for_v_tasks << std::endl; 
	return ACEMESH_OK;
}

void task_dag_graph::add_tuples_to_virtual_task(aceMesh_task* t, void* addr, int type, int significance)
{
    tuple_addr_task  tmp;
    tmp.t = t;
    tmp.type = type;
    tmp.addr = addr;
    tmp.significance = significance;
    this->one_virtual_task_tuples.push_back(tmp);
}

#ifdef USE_PRIORITY_QUEUE
bool compare_task2(aceMesh_task* t1, aceMesh_task* t2)
{ 
    //large than...., same as compare_task in class generic_scheduler
    return t1->get_priority_id() > t2->get_priority_id();
}
#endif
Error_Code task_dag_graph::spawn()
{
#ifdef USE_PRIORITY_QUEUE
	if(0){
      //it is not necessary to sort the list before entering a red-black tree
      std::sort(need_spawn_tasks.begin(), need_spawn_tasks.end(), compare_task2);
	}
#endif
    //TODO lyt
    endTask->increment_ref_count();

	for(std::vector<aceMesh_task*>::iterator itr = need_spawn_tasks.begin(); itr != need_spawn_tasks.end(); ++itr)
	{
        //std::cout<<"tasks_num:" << (*itr)->get_successor_sum() << std::endl;
        //std::cout<<"ref_count:" << (*itr)->ref_count() << std::endl;
        //assert((*itr)->ref_count() == 0);
        //std::cout << "priority : " << (*itr)->get_priority_id() << std::endl;        
#if defined(DYNAMIC_SCHEDULER)
	//std::cout<<"DYNAMIC_SCHEDULER" <<std::endl;
  #ifdef NOT_MPI_STRATEGY
		task::spawn(**itr);
		//std::cout<<"NOT_MPI_STRATEGY" <<std::endl;
  #else
	  bool blocking_flag  = (((aceMesh_task*)(*itr))->get_task_type()==BLOCKING_TASK);
	  if(!blocking_flag) {
	    task::spawn(**itr);
	  }
	  else {
	    #ifdef EXTRA_THREAD
	      task::mpi_spawn(**itr);
	    #endif
	    #ifdef BLOCKING_QUEUE
	      task::spawn(**itr);
	    #endif
	  }
	#endif
#else
    #ifdef NOT_MPI_STRATEGY
      task::init_spawn(**itr);
	  //std::cout<<"NOT_MPI_STRATEGY_DYNAMIC_SCHEDULER" <<std::endl;
    #else
      bool blocking_flag  = (((aceMesh_task*)(*itr))->get_task_type()==BLOCKING_TASK);
      if (!blocking_flag) {
        task::init_spawn(**itr);
      }
      else {
        #ifdef EXTRA_THREAD
        task::mpi_spawn(**itr);
        #endif
        
        #ifdef BLOCKING_QUEUE
        task::init_spawn(**itr);
        #endif
      }
      #endif
  #endif
  }
  return ACEMESH_OK;
}


Error_Code task_dag_graph::wait()//not use
{
	this->endTask->wait_for_all();
	this->reset_task_graph();
	return ACEMESH_OK;
}

#ifdef AUTO_PARTITION
void task_dag_graph::spawn(int sep_id)
{
    std::vector<int>& my_sep_data = sep_data[sep_id];

    for(std::vector<int>::iterator itr = my_sep_data.begin(); itr != my_sep_data.end(); ++itr)
    {
        task::spawn(*sep_task[*itr]);
    }
}

void task_dag_graph::end_construct_dag_separation()
{
    sep_end_tasks.clear();
    for(std::unordered_set<aceMesh_task*>::iterator itr = this->end_tasks.begin(); 
        itr != this->end_tasks.end(); ++itr)
    {
        //std::cout << (*itr)->get_group_id();
        int id = my_disjoin_set.find_set( (*itr)->get_group_id() );
        std::map<int, task*>::iterator itr_find = sep_end_tasks.find(id);
        task* tmp = NULL;
        if(itr_find == sep_end_tasks.end())
        {
#ifdef DEBUG_GRAPH
            tmp = ACEMESH_NEW end_task();
#else
            tmp = ACEMESH_NEW empty_task();
#endif
            sep_end_tasks.insert(std::pair<int,task*>(id, tmp));
        }
        else 
        {
            tmp = itr_find->second;
        }
        //std::cout<< "is joint : " << itr->t->is_joint << std::endl;
        (*itr)->add_end_successor(tmp);
    }
    //std::cout<<"endTask ref_count "<<endTask->ref_count() << std::endl;
}

Error_Code task_dag_graph::spawn_and_wait_with_separation()
{
    int sep_num = my_disjoin_set.get_sep_data(sep_data);
    this->end_construct_dag_separation();

#ifdef _EXEC_TIME
    tbb::tick_count mainStartTime = tbb::tick_count::now();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
    acemesh_papi_start_perf_cnt();
#endif

    //std::cout << "this graph has " << sep_num << " partition" << std::endl;
    int i = 0;
    for(std::map<int, std::vector<int> >::iterator map_itr = sep_data.begin(); map_itr != sep_data.end(); ++map_itr)
    {
        std::vector<int>& my_sep_data = map_itr->second;
        //std::cout << "the " << i++ << "th part has "<< my_sep_data.size() << " tasks" << std::endl;

        int id = map_itr->first;
        task* my_part = sep_end_tasks[id];
        my_part->increment_ref_count();
        for(std::vector<int>::iterator itr = my_sep_data.begin(); itr != my_sep_data.end(); ++itr)
        {
            task::spawn(*sep_task[*itr]);
        }

        my_part->wait_for_all();

    }

#ifdef _EXEC_TIME
    tbb::tick_count mainEndTime = tbb::tick_count::now();
	aceMesh_task::exec_time += (mainEndTime - mainStartTime).seconds();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
    acemesh_papi_end_perf_cnt();
    long long cache_miss = acemesh_papi_perf_cnt();
    aceMesh_task::cache_miss += cache_miss;
#endif


    this->reset_task_graph();
    sep_id = 0;
    my_disjoin_set.clear();
    return ACEMESH_OK;
}
#endif

#ifdef DEBUG_GRAPH
Error_Code task_dag_graph::spawn_and_wait(int print_graph)
{
    this->g_print_graph = true;
    return this->spawn_and_wait();
}
#endif

void task_dag_graph::_store_dag_info()
{
        for(std::vector<aceMesh_task*>::iterator itr = need_spawn_tasks.begin(); itr != need_spawn_tasks.end(); ++itr) {
            (*itr)->store_info();
        }
}

void task_dag_graph::free_tasks()
{
    //endTask not instance of aceMesh_task, can not cast it
    for (auto mapitr = dag_instance_map.begin(); mapitr != dag_instance_map.end(); ++mapitr) {
        need_spawn_tasks = mapitr->second.need_spawn_tasks;
        wait_free_tasks.clear();
        for (std::vector<aceMesh_task *>::iterator itr = need_spawn_tasks.begin(); itr != need_spawn_tasks.end(); ++itr)
        {
            (*itr)->get_free_list(wait_free_tasks);
        }
        for (auto itr = wait_free_tasks.begin(); itr != wait_free_tasks.end(); ++itr)
        {
            delete (*itr);
        }
    }
}

void task_dag_graph::_save_to_buffer()
{
    dag_instance tmp_instance = {
        this->endTask,
        this->need_spawn_tasks
    };
    dag_instance_map[curr_dagNo] = tmp_instance;
}
void task_dag_graph::_resume_from_buffer()
{
    dag_instance tmp_instance = dag_instance_map[curr_dagNo]; 
    this->endTask = tmp_instance.endTask;
    this->need_spawn_tasks = tmp_instance.need_spawn_tasks;
}

Error_Code task_dag_graph::spawn_and_wait()
{
//#ifdef AUTO_PARTITION 
//    return spawn_and_wait_with_separation();
//#endif
    bool reuse = false;
    bool init = false;

    auto iter = dag_reuse_flag.find(curr_dagNo);
    if (iter != dag_reuse_flag.end()) {
       reuse = iter->second; 
       init = dag_reuse_cnt[curr_dagNo] == -1 ? true : false; 
    }

    if (reuse) {//when dag_start return 1
        //resume dag
        this->_resume_from_buffer();
        //Store refcount in task, Now restore when exec
        //this->_restore_dag_info();
    } else {
        this->end_construct_dag();
        if (init) {
            dag_reuse_cnt[curr_dagNo] = 0;
            this->_store_dag_info();
            //save dag to buffer
            this->_save_to_buffer();
        }
    }

#ifdef _DAG_TIME
    //count construct time
    dagEndTime = tbb::tick_count::now();
    if (reuse) {
        dag_reuse_time += (dagEndTime - dagStartTime).seconds();
        std::cout << "----dag reuse time: " << dag_reuse_time << std::endl;
    } else {
        dag_build_time += (dagEndTime - dagStartTime).seconds();
        std::cout << "----dag build time: " << dag_build_time << std::endl;
    }

    dagStartTime = tbb::tick_count();
#endif

#ifdef _EXEC_TIME
    tbb::tick_count mainStartTime = tbb::tick_count::now();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
    acemesh_papi_start_perf_cnt();
#endif
    // std::cout << "before_spawn" << std::endl;
    std::cout << "endTask refcount: " << this->endTask->ref_count() << std::endl;
	this->spawn();

    // std::cout << "before_wait_for_all" << std::endl;
    this->endTask->wait_for_all();
    // std::cout << "end_wait_for_all" << std::endl;
    // std::cout << "endTask refcount: " << this->endTask->ref_count() << std::endl;

#ifdef _EXEC_TIME
    tbb::tick_count mainEndTime = tbb::tick_count::now();
	aceMesh_task::exec_time += (mainEndTime - mainStartTime).seconds();
    std::cout<< "--exec time : " << aceMesh_task::exec_time << std::endl;
//    if(my_mpi_rank<=0)
//      std::cout << "each_dag_exec_time: " << (mainEndTime - mainStartTime).seconds() <<std::endl;
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
    acemesh_papi_end_perf_cnt();
    long long cache_miss = acemesh_papi_perf_cnt();
    printf("cache_miss : %lld\n",cache_miss);
    aceMesh_task::cache_miss += cache_miss;
    std::cout<<aceMesh_task::cache_miss<<std::endl;
#endif

    //std::cout<<"exec time:"<< (mainEndTime - mainStartTime).seconds() << std::endl;
    if (this->endTask->get_reused_flag()) {
        this->endTask->restore_ref_count(); //for reuse restore
    }
    this->reset_task_graph();

#ifdef AUTO_PARTITION
    sep_id = 0;
    my_disjoin_set.clear();
#endif
    
	return ACEMESH_OK;
}

#ifdef CONCURRENT_CONSTRUCT_GRAPH
Error_Code task_dag_graph::wait_for_all_task()
{
    this->end_construct_dag();

#ifdef ACEMESH_PERFORMANCE
    my_perf.record_start();
#endif

    endTask->increment_ref_count();
    this->endTask->wait_for_all();

#ifdef ACEMESH_PERFORMANCE
    my_perf.record_end();
#endif

#ifdef _EXEC_TIME
    mainEndExecTime = tbb::tick_count::now();
    exec_time = exec_time + (mainEndExecTime - mainStartExecTime).seconds();
#endif

    this->reset_task_graph();
    std::vector<aceMesh_task *>::iterator same_itr;
    std::vector<aceMesh_task *>::iterator copy_same_itr;
    std::vector<aceMesh_task *>::iterator del_same_itr;
    std::vector<aceMesh_task *> del_vec;
    //empty_task* endTask_check;
    //del_vec.push_back(endTask_check);
    for (std::unordered_map<void *, tuple_rw_task>::iterator search_itr = this->addr_task.begin();
         search_itr != this->addr_task.end(); ++search_itr)
    {
        if (search_itr->second.w_task.t != NULL)
        {
            for (same_itr = del_vec.begin(); same_itr != del_vec.end(); ++same_itr)
            {
                //copy_same_itr = same_itr;
                // std::cout<<"-----------------------first--------------------------------------------------"<<std::endl;
                if (((*same_itr)->get_loop_id()) == (search_itr->second.w_task.t->get_loop_id()) &&
                    ((*same_itr)->get_task_id()) == (search_itr->second.w_task.t->get_task_id()))
                    break;
            }
            if (same_itr == del_vec.end())
            {
                del_vec.push_back(search_itr->second.w_task.t);
                // std::cout<<"del_vec size first:"<<del_vec.size()<<std::endl;
            }
        }
        for (std::vector<task_significance>::iterator r_task_itr = search_itr->second.r_tasks.begin();
             r_task_itr != search_itr->second.r_tasks.end(); ++r_task_itr)
        {
            for (copy_same_itr = del_vec.begin(); copy_same_itr != del_vec.end(); ++copy_same_itr)
            {
                //copy_same_itr = same_itr;
                //std::cout<<"--------------------second-------------------------------------------------"<<std::endl;
                if (((*copy_same_itr)->get_loop_id()) == (r_task_itr->t->get_loop_id()) &&
                    ((*copy_same_itr)->get_task_id()) == (r_task_itr->t->get_task_id()))
                    break;
            }
            if (copy_same_itr == del_vec.end())
            {
                //std::cout<<"same_itr loop id:"<<(*copy_same_itr)->get_loop_id()<<"same_itr task id:"<<(*copy_same_itr)->get_task_id()<<std::endl;
                //std::cout<<"r_task_itr loop id:"<<r_task_itr->t->get_loop_id()<<"r_task_itr task id:"<<r_task_itr->t->get_task_id()<<std::endl;
                del_vec.push_back(r_task_itr->t);
                //	std::cout<<"del_vec size second:"<<del_vec.size()<<std::endl;
            }
        }
    }
    for (del_same_itr = del_vec.begin(); del_same_itr != del_vec.end(); ++del_same_itr)
    {
        //std::cout<<"-----------------------three-------------------------------------------------"<<std::endl;
        //  std::cout<<"del_vec size:"<<del_vec.size()<<std::endl;
        delete *del_same_itr;
        *del_same_itr = NULL;
    }
#ifdef AUTO_PARTITION
    sep_id = 0;
    my_disjoin_set.clear();
#endif

    return ACEMESH_OK;
}
#endif

Error_Code task_dag_graph::reset_affinity()
{
#ifdef AUTO_AFFINITY
    my_affinity.reset_affinity_id();
#endif
    return ACEMESH_OK;
}

Error_Code task_dag_graph::begin_split_task()
{    
	this->one_virtual_task_tuples.clear();
#ifndef ALL_CYCLIC
#ifdef AUTO_AFFINITY
    my_affinity.reset_affinity_id();
#endif
#endif
    
	return ACEMESH_OK;
}

#define WRITE 1
#define READ  2
int task_dag_graph::update_write_addr(std::map<void*,update_addr_flag>& addr_update, void* addr, int type)
{
    std::map<void*,update_addr_flag>::iterator itr = addr_update.find(addr);
    if(itr == addr_update.end())
    {
        update_addr_flag update_info; 
        if(type == WRITE) 
        {
            update_info.w_flag = true;
        } 
        else if(type == READ) 
        {
            update_info.r_flag = true;
        }
        addr_update.insert( std::map<void*,update_addr_flag>::value_type(addr, update_info) );
    }
    else 
    {
        update_addr_flag& tmp = itr->second;
        if(type == WRITE && tmp.w_flag == true)
        {
            tmp.w_flag = true;
            return 2;
        }
        if(type == WRITE && tmp.r_flag == true)
        {
            tmp.w_flag = true;
            return 1;
        }
        if(type == READ && tmp.w_flag == true)
        {
            tmp.r_flag = true;
            return 1;
        }

        if(type == WRITE) 
        {
            tmp.w_flag = true;
        } 
        else if(type == READ) 
        {
            tmp.r_flag = true;
        }
    }
    return 0;
}

bool cmp_tuple(const tuple_addr_task& tuple1, const tuple_addr_task& tuple2)
{
    if( tuple1.addr < tuple2.addr ) 
        return true;
    if( tuple1.addr == tuple2.addr && tuple1.t < tuple2.t )
        return true;
    return false;
}
int task_dag_graph::type_add(int type1, int type2)
{
    if(type1 == INOUT || type2 == INOUT)
        return INOUT;
    if(type1 == OUT || type2 == OUT)
    {
        if(type1 == IN && type2 == OUT || type1 == OUT && type2 == IN)
            return INOUT;
        return OUT;
    }
    if (type1 == IN || type2 == IN) 
        return IN;
    else 
        return INOUT_NONE;
}
int task_dag_graph::significance_add(int significance1, int significance2)
{
    return significance1 && significance2;
}
void task_dag_graph::unique_tuples()
{
    std::vector<tuple_addr_task>::iterator tmp_itr;
	for(std::vector<tuple_addr_task>::iterator itr = this->one_virtual_task_tuples.begin(); 
		itr != this->one_virtual_task_tuples.end(); )
    {
        tmp_itr = itr;
        ++itr;
        for(; itr != this->one_virtual_task_tuples.end(); ++itr)
        {
            if(itr->addr != tmp_itr->addr || itr->t != tmp_itr->t )
            {
                break;
            }
            else 
            {
                itr->type =  type_add(itr->type, tmp_itr->type);
                itr->significance = significance_add(itr->significance, tmp_itr->significance);

                tmp_itr->addr = NULL;
                tmp_itr->t = NULL;
                tmp_itr = itr;
            }
        }
    }
}
Error_Code task_dag_graph::end_split_task()
{
#ifdef CONCURRENT_CONSTRUCT_GRAPH
    static unsigned int called_cnt = 1;//log call times of this function
#endif

#ifdef MERGE_SAME_ADDR
    std::sort(this->one_virtual_task_tuples.begin(), this->one_virtual_task_tuples.end(), cmp_tuple);
    //std::stable_sort(this->one_virtual_task_tuples.begin(), this->one_virtual_task_tuples.end(), cmp_tuple);
    this->unique_tuples();
#endif

    //std::cout<<"end split"<<std::endl;
#ifdef DEBUG_GRAPH
    std::map<void*,update_addr_flag> addr_update;
#endif

	for(std::vector<tuple_addr_task>::iterator itr = this->one_virtual_task_tuples.begin(); 
		itr != this->one_virtual_task_tuples.end(); ++itr)
	{
        if(itr->type == INOUT_NONE || itr->addr == NULL || itr->t == NULL)
            continue;
#ifdef DEBUG_GRAPH
        int confilt = 0;
#endif
		std::unordered_map<void*, tuple_rw_task>::iterator search_itr  = this->addr_task.find(itr->addr);
		if(search_itr == addr_task.end())
		{
			rw_task_tuple tmp;
			if(itr->type == OUT || itr->type == INOUT)
			{
				tmp.w_task.t = itr->t;
                tmp.w_task.significance = itr->significance;
#ifdef DEBUG_GRAPH
                confilt = this->update_write_addr(addr_update, itr->addr, WRITE);
#endif
			} 
			else if( itr->type == IN )
			{
                task_significance t_s;
                t_s.t = itr->t;
                t_s.significance = itr->significance;
				tmp.r_tasks.push_back(t_s);
#ifdef DEBUG_GRAPH
                confilt = this->update_write_addr(addr_update, itr->addr, READ);
#endif
			}
            else 
            {
                assert(0);
            }
			this->addr_task[itr->addr] = tmp; 

            //std::cout<<"insert addr :" <<  itr->addr << std::endl;
		}
		else
		{
			if(itr->type == OUT || itr->type == INOUT)
			{
				search_itr->second.w_task.t = itr->t;
                search_itr->second.w_task.significance = itr->significance;
				search_itr->second.r_tasks.clear();
#ifdef DEBUG_GRAPH
                confilt = this->update_write_addr(addr_update, itr->addr, WRITE);
#endif
			}
			else 
			{
                task_significance t_s;
                t_s.t = itr->t;
                t_s.significance = itr->significance;
				search_itr->second.r_tasks.push_back(t_s);
#ifdef DEBUG_GRAPH
                confilt = this->update_write_addr(addr_update, itr->addr, READ);
#endif
			}
		}
#ifdef DEBUG_GRAPH
        if(confilt)
        {
            std::cout<< " warning: this for_all loop (loop name : " << graph_check.get_last_loop_info() 
                << ") has read and write the same addr(block) " << itr->addr << std::endl;
            std::cout<< (confilt==1 ? "Read in one task, Write in another task" : 
                "Write in one task, Write in another task") << std::endl;
            graph_check.print_stack() ;
		    //std::unordered_map<void*, tuple_rw_task>::iterator search_itr  = this->addr_task.find(itr->addr);
		    if(search_itr != addr_task.end())
            {
                search_itr->second.r_tasks.clear();
            }
        }
#endif
	}

#ifdef CONCURRENT_CONSTRUCT_GRAPH
    this->spawn();
    need_spawn_tasks.clear();
    if (called_cnt == 1)
        mainStartExecTime = tbb::tick_count::now();
#endif

	return ACEMESH_OK;
}

Error_Code task_dag_graph::end_construct_dag()
{

#ifdef DEBUG_GRAPH
	endTask = ACEMESH_NEW end_task();
    //std::cout<< "is joint : " << endTask->is_joint << std::endl;
#else
	endTask = ACEMESH_NEW empty_task();
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
    for (std::unordered_set<aceMesh_task *>::iterator itr = end_tasks.begin();
         itr != end_tasks.end(); ++itr)
    {
        (*itr)->finished_lock.lock();
        //        if((*itr)->finish == false)
        if ((*itr)->finish == false)
        {
            (*itr)->add_end_successor(endTask);
        }
        (*itr)->finished_lock.unlock();
        //std::cout<< "finished_status " << (*itr)->is_ready_task() << std::endl;
    }
    std::cout << "endTask ref_count " << endTask->ref_count() << std::endl;
#else
	for(std::unordered_set<aceMesh_task*>::iterator itr = this->end_tasks.begin(); 
		itr != this->end_tasks.end(); ++itr)
	{
        //std::cout<< "is joint : " << itr->t->is_joint << std::endl;
		(*itr)->add_end_successor(endTask);
	}
    // std::cout<<"endTask ref_count "<<endTask->ref_count() << std::endl;
#endif

#ifdef DEBUG_GRAPH
    if(this->g_print_graph)
    {
        this->check_graph(1);
        this->g_print_graph = false;
    }
    else
        this->check_graph();
#endif
	return ACEMESH_OK;
}



void task_dag_graph::reset_task_graph()
{
	need_spawn_tasks.clear();
    addr_task.clear();
    end_tasks.clear();
#ifdef DEBUG_GRAPH
    task_nums = 0;

#endif

#if defined(ACEMESH_SCHEDULER_PROFILING_EACH_TASK)  || defined(DEBUG_GRAPH)
  aceMesh_task::loop_names.clear();
#endif    
    
#ifdef SUPPORT_PARTITION
    sep_data.clear();
#endif
}

void task_dag_graph::init_thread_num(int thread_num)
{
    this->thread_num = thread_num;
}

#ifndef SPECIFY_END_TASKS
inline void task_dag_graph::add_end_task(aceMesh_task* t)
{
    end_tasks.insert(t);
    //end_tasks.push_back(t);
}

inline void task_dag_graph::del_end_task(aceMesh_task* t)
{
    std::unordered_set<aceMesh_task*>::iterator itr = end_tasks.find(t);
    if( itr == end_tasks.end() )
    {
        return ;
    }
    end_tasks.erase(itr);

    //for(std::list<aceMesh_task*>::iterator itr = end_tasks.begin(); itr != end_tasks.end(); ++itr)
    //{/
    //    if(*itr == t)
    //    {
    //        end_tasks.erase(itr);
    //        return;
    //    }
    //}
}
#endif

Error_Code task_dag_graph::begin_split_task(const std::string& loop_info)
{
#ifdef _DAG_TIME
    if ((dagStartTime-tbb::tick_count()).seconds()==0)
        dagStartTime = tbb::tick_count::now();
#endif
#if defined(ACEMESH_SCHEDULER_PROFILING_EACH_TASK)  || defined(DEBUG_GRAPH)
    aceMesh_task::loop_names.push_back(loop_info);
#endif
#ifdef DEBUG_GRAPH
    graph_check.add_loop_info(loop_info);
#endif
	this->one_virtual_task_tuples.clear();
#ifdef AUTO_AFFINITY
    my_affinity.set_thread_num(sche_num_threads);    
#endif
#ifndef ALL_CYCLIC
#ifdef AUTO_AFFINITY
    my_affinity.reset_affinity_id();
#endif
#endif

    return ACEMESH_OK;
}

#ifdef SWF 
void task_dag_graph::push_neighbor_addr(void *addr) {
  if (num_neighbs >= MAX_NEIGHBORS) {
    printf("Too many neighbors:%d\n", num_neighbs);
    exit(0);
  }  
  neighbor_addrs[num_neighbs++]=addr;
}
#endif
#ifdef DEBUG_GRAPH
//only support seq exec
bool task_dag_graph::check_graph()
{
    std::cout<< "acemesh :: begining check " << graph_check.get_last_loop_info()  << "\n";
    graph_check.check_graph_bfs(need_spawn_tasks, task_nums);
    return true;
    //return graph_check.check_graph_dfs(need_spawn_tasks, task_nums);
}
//only support seq exec
bool task_dag_graph::check_graph(int print_graph)
{
    std::cout<< "acemesh :: begining check " << graph_check.get_last_loop_info()  << "\n";
    graph_check.print_loop_depedence(need_spawn_tasks, task_nums);
    return true;
    //return graph_check.check_graph_dfs(need_spawn_tasks, task_nums);
}
#endif

#ifdef SUPPORT_PARTITION
Error_Code task_dag_graph::spawn_and_wait_with_separation()
{
    this->end_construct_dag_separation();


    //std::cout << "this graph has " << sep_data.size() << " partition" << std::endl;
    int i = 0;
    for(std::map<int, std::vector<aceMesh_task*> >::iterator map_itr = sep_data.begin(); map_itr != sep_data.end(); ++map_itr)
    {
        std::vector<aceMesh_task*>& my_sep_data = map_itr->second;
        //std::cout << "the " << i++ << "th part has "<< my_sep_data.size() << " tasks" << std::endl;

        int id = map_itr->first;
        task* my_part = sep_end_tasks[id];
        my_part->increment_ref_count();
#ifdef _EXEC_TIME
    tbb::tick_count mainStartTime = tbb::tick_count::now();
#endif
#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
    acemesh_papi_start_perf_cnt();
#endif
        for(std::vector<aceMesh_task*>::iterator itr = my_sep_data.begin(); itr != my_sep_data.end(); ++itr)
        {
            task::spawn(**itr);
        }

        my_part->wait_for_all();

#ifdef _EXEC_TIME
    tbb::tick_count mainEndTime = tbb::tick_count::now();
	aceMesh_task::exec_time += (mainEndTime - mainStartTime).seconds();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
    acemesh_papi_end_perf_cnt();
    long long cache_miss = acemesh_papi_perf_cnt();
    aceMesh_task::cache_miss += cache_miss;
#endif

    }


    this->reset_task_graph();
    return ACEMESH_OK;
}
void task_dag_graph::end_construct_dag_separation()
{
    sep_end_tasks.clear();
    for(std::unordered_set<aceMesh_task*>::iterator itr = this->end_tasks.begin(); 
        itr != this->end_tasks.end(); ++itr)
    {
        //std::cout << (*itr)->get_group_id();
        int id =  (*itr)->get_part_id() ;
        std::map<int, task*>::iterator itr_find = sep_end_tasks.find(id);
        task* tmp = NULL;
        if(itr_find == sep_end_tasks.end())
        {
#ifdef DEBUG_GRAPH
            tmp = ACEMESH_NEW end_task();
#else
            tmp = ACEMESH_NEW empty_task();
#endif
            sep_end_tasks.insert(std::pair<int,task*>(id, tmp));
        }
        else 
        {
            tmp = itr_find->second;
        }
        //std::cout<< "is joint : " << itr->t->is_joint << std::endl;
        (*itr)->add_end_successor(tmp);
    }
#ifdef DEBUG_GRAPH
    if(this->g_print_graph)
    {
        this->check_graph(1);
        this->g_print_graph = false;
    }
    else
        this->check_graph();
#endif
    //std::cout<<"endTask ref_count "<<endTask->ref_count() << std::endl;
}

//void task_dag_graph::set_separations(std::map<void*, int>& sep_datas)
//{
//    addr_sep_datas = sep_datas;
//}
#endif

}
