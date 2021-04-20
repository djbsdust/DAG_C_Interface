#include "concurrent_task_dag_graph.h"
#include <iostream>
#include <cassert>
#include <map>
#include <algorithm>
#include "tbb/tick_count.h"
#include "addr_area_manager.h"

namespace AceMesh_runtime {
//int time_for_v_tasks = 0;
#ifdef _EXEC_TIME
double exec_time = 0.0;
#endif

concurrent_task_dag_graph::concurrent_task_dag_graph()
{
}

concurrent_task_dag_graph::~concurrent_task_dag_graph()
{
#ifdef _EXEC_TIME
    std::cout<< "exec time : " << exec_time << std::endl;
#endif

}
concurrent_task_dag_graph::concurrent_task_dag_graph(int thread_nums)
{
}

void concurrent_task_dag_graph::init(int thread_num)
{
}

void concurrent_task_dag_graph::insert_to_addr_hash(aceMesh_task* t, int type, void* addr, int significance, 
        /* out */ std::pair<tbb::concurrent_unordered_map<void*, tuple_rw_task>::iterator, bool>& result)
{
    rw_task_tuple tmp;
    if( type == OUT || type == INOUT)
    {
        tmp.w_task.t = t;
        tmp.w_task.significance = significance;
    } 
    else if( type == IN )
    {
        task_significance t_s;
        t_s.t = t;
        t_s.significance = significance;
        tmp.r_tasks.push_back(t_s);
    }
    else 
    {
        assert(0);
    }
    result = this->addr_task.insert(std::pair<void*,tuple_rw_task>(addr,tmp));
}

void concurrent_task_dag_graph::update_to_addr_hash(aceMesh_task* t, int type, int significance, tuple_rw_task& second)
{
    if( type == OUT || type == INOUT)
    {
        second.w_task.t = t;
        second.w_task.significance = significance;
        second.r_tasks.clear();
    }
    else 
    {
        task_significance t_s;
        t_s.t = t;
        t_s.significance = significance;
        second.r_tasks.push_back(t_s);
    }
}

inline int concurrent_task_dag_graph::build_releationship
    (task* dest, int type, tuple_rw_task& src, bool is_neighbor) 
{
    int res = 0;
	aceMesh_task* tmp = NULL;
	if(src.r_tasks.size() == 0)
	{
		if(src.w_task.t != NULL)
		{
			tmp = src.w_task.t;
            if( !is_neighbor && src.w_task.significance == 0)
            {
                if(type != INOUT_NONE)
                {
                    del_end_task(tmp);
                    tmp->set_vertical_task(dest);
                    ++res;
                    //time_for_v_tasks++; 
                }
            }
            else if(is_neighbor || src.w_task.significance == 1)
            {
                if(type != INOUT_NONE)
                {
                    del_end_task(tmp);
			        tmp->add_successor(dest);
                    ++res;
                }
            }
            else 
            {
                assert(0);
            }
            //std::cout<<"add edge\n";
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
				tmp = src.w_task.t;
                if(!is_neighbor && src.w_task.significance == 0)
                {
                    if(type != INOUT_NONE)
                    {
                        //time_for_v_tasks++; 
                        del_end_task(tmp);
                        tmp->set_vertical_task(dest);
                        ++res;
                    }
                }
                else if( is_neighbor || src.w_task.significance == 1)
                {
                    if(type != INOUT_NONE)
                    {
                        del_end_task(tmp);
                        tmp->add_successor(dest);
                        ++res;
                    }
                }
                else 
                {
                    assert(0);
                }
                //std::cout<<"add edge\n";
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
            for(tbb::concurrent_vector<task_significance>::iterator r_task_itr = src.r_tasks.begin(); 
                    r_task_itr != src.r_tasks.end(); ++r_task_itr)
            {
                if(!is_neighbor && (*r_task_itr).significance == 0)
                {
                    if(type != INOUT_NONE)
                    {
                        del_end_task(r_task_itr->t);
                        r_task_itr->t->set_vertical_task(dest);
                        ++res;
                        //time_for_v_tasks++; 
                    }
                }
                else if( is_neighbor || (*r_task_itr).significance == 1)
                {
                    if(type != INOUT_NONE)
                    {
                        del_end_task(r_task_itr->t);
                        r_task_itr->t->add_successor(dest);
                        ++res;
                    }
                }
                else 
                {
                    assert(0);
                }
			    //(*r_task_itr)->add_successor(t);
                //std::cout<<"add edge\n";
            }
		}
    }
    return res;
}

inline void* concurrent_task_dag_graph::get_true_addr(void* addr, int type)
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

Error_Code concurrent_task_dag_graph::register_task(aceMesh_task* t, std::vector<addr_tuple >& addrs)
{
    //reuse this var 
#ifndef    
    std::vector<void*> neighbor_addrs;
#else
  void* neighbor_addrs[MAX_NEIGHBORS];    
  int num_neighbs;
#endif

    add_end_task(t);

	int sum_pre = 0;
    //time_for_v_tasks = 0;
    std::pair<tbb::concurrent_unordered_map<void*, tuple_rw_task>::iterator, bool> result;
	for(std::vector<addr_tuple>::iterator itr = addrs.begin(); itr != addrs.end(); ++itr)
	{
        //TODO: add shade and unshade area's  depend  to narmal  
        if (itr->addr != NULL)
        {
            //get true inout addr
            void* true_addr = this->get_true_addr(itr->addr, itr->area_type);

            insert_to_addr_hash(t, itr->type, true_addr, 0, result);
            //insert fail
	        if( !result.second )
	        { 
                sum_pre += build_releationship(t, itr->type, result.first->second, false);
                update_to_addr_hash(t, itr->type, 0, result.first->second);
	        }
        }
	    if( itr->neighbor == NEIGHBOR_NONE )
            continue;

#ifndef SWF      
      neighbor_addrs.len=0;
      neighbor_addrs.clear();
      //note: this para is itr->addr
      t->get_neighbor(itr->neighbor, itr->addr, &neighbor_addrs);
#else
      num_neighbs=0;
      t->get_neighbor();
#endif 


#ifndef SWF
      //get neighbors tasks and build releationship      
      for(std::vector<void*>::iterator neighbor_itr = neighbor_addrs.begin(); 
              neighbor_itr != neighbor_addrs.end(); ++neighbor_itr) {
          //get true addr
          void* neighbor_true_addr = this->get_true_addr(*neighbor_itr, itr->area_type);

          insert_to_addr_hash(t, itr->neighbor_type, neighbor_true_addr, 1, result);
          if( result.second ) { 
            continue;
          }
          sum_pre += build_releationship(t, itr->neighbor_type, result.first->second, true);
          update_to_addr_hash(t, itr->neighbor_type, 1, result.first->second);
      }
#else
      for(int i=0; i<num_neighbs; i++) {
        //get true addr
          void* neighbor_true_addr = this->get_true_addr(neighbor_addrs[i], itr->area_type);

          insert_to_addr_hash(t, itr->neighbor_type, neighbor_true_addr, 1, result);
          if( result.second ) { 
            continue;
          }
          sum_pre += build_releationship(t, itr->neighbor_type, result.first->second, true);
          update_to_addr_hash(t, itr->neighbor_type, 1, result.first->second);            
      }
#endif
      if (sum_pre == 0) {
        //note: here, I doesn't judge t whether this task is already in vector
        //so, if this vector has two tasks in same 
        //tbb lib will give error
        need_spawn_tasks.push_back(t);
      }
    //std::cout << "v_task : " << time_for_v_tasks << std::endl; 
    return ACEMESH_OK;
}


Error_Code concurrent_task_dag_graph::spawn()
{
    join_task->increment_ref_count();
	for(tbb::concurrent_vector<aceMesh_task*>::iterator itr
				= need_spawn_tasks.begin(); itr != need_spawn_tasks.end(); ++itr)
	{
#if defined(DYNAMIC_SCHEDULER)
  #ifdef NOT_MPI_STRATEGY
		task::spawn(**itr);
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
#endif
	}
	return ACEMESH_OK;
}


Error_Code concurrent_task_dag_graph::wait()
{
	join_task->wait_for_all();
	this->reset_task_graph();
	return ACEMESH_OK;
}
Error_Code concurrent_task_dag_graph::spawn_and_wait()
{
    //SOME
    this->end_construct_dag();
#ifdef _EXEC_TIME
    tbb::tick_count mainStartTime = tbb::tick_count::now();
#endif

	this->spawn();
	join_task->wait_for_all();

#ifdef _EXEC_TIME
    tbb::tick_count mainEndTime = tbb::tick_count::now();
    exec_time += (mainEndTime - mainStartTime).seconds();
#endif
    //std::cout<<"exec time:"<< (mainEndTime - mainStartTime).seconds() << std::endl;
    //
	this->reset_task_graph();
    
	return ACEMESH_OK;
}

Error_Code concurrent_task_dag_graph::begin_split_task()
{
    
	return ACEMESH_OK;
}

#ifdef DEBUG_GRAPH
Error_Code concurrent_task_dag_graph::spawn_and_wait(int print_graph)
{
    //TODO:
	return ACEMESH_OK;

}
#endif

#define WRITE 1
#define READ  2
int concurrent_task_dag_graph::update_write_addr(std::map<void*,update_addr_flag>& addr_update, void* addr, int type)
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
int concurrent_task_dag_graph::type_add(int type1, int type2)
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
int concurrent_task_dag_graph::significance_add(int significance1, int significance2)
{
    return significance1 && significance2;
}
Error_Code concurrent_task_dag_graph::end_split_task()
{
	return ACEMESH_OK;
}

Error_Code concurrent_task_dag_graph::end_construct_dag()
{

	join_task = ACEMESH_NEW empty_task();

	for(std::unordered_set<aceMesh_task*>::iterator itr = this->end_tasks.begin(); 
		itr != this->end_tasks.end(); ++itr)
	{
        //std::cout<< "is joint : " << itr->t->is_joint << std::endl;
		(*itr)->add_successor(join_task);
	}
	return ACEMESH_OK;
}



void concurrent_task_dag_graph::reset_task_graph()
{
	need_spawn_tasks.clear();
    addr_task.clear();
    end_tasks.clear();
#ifdef DEBUG_GRAPH
    task_nums = 0;
#endif
}

inline void concurrent_task_dag_graph::add_end_task(aceMesh_task* t)
{
    mutex_end_tasks.lock();
    end_tasks.insert(t);
    mutex_end_tasks.unlock();
}

inline void concurrent_task_dag_graph::del_end_task(aceMesh_task* t)
{
    //TODO, opt it
    mutex_end_tasks.lock();
    end_tasks.erase(t);
    mutex_end_tasks.unlock();
}

#ifdef DEBUG_GRAPH
Error_Code concurrent_task_dag_graph::begin_split_task(const std::string& loop_info)
{
    graph_check.add_loop_info(loop_info);

	this->one_virtual_task_tuples.clear();
	return ACEMESH_OK;
}
//only support seq exec
bool concurrent_task_dag_graph::check_graph()
{
    std::cout<< "acemesh :: begining check\n";
    graph_check.check_graph_bfs(need_spawn_tasks, task_nums);
    return true;
    //return graph_check.check_graph_dfs(need_spawn_tasks, task_nums);
}
//only support seq exec
bool concurrent_task_dag_graph::check_graph(int print_graph)
{
    std::cout<< "acemesh :: begining check\n";
    graph_check.print_loop_depedence(need_spawn_tasks, task_nums);
    return true;
    //return graph_check.check_graph_dfs(need_spawn_tasks, task_nums);
}
#endif
}
