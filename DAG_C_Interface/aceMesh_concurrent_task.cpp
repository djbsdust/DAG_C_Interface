#include "aceMesh_concurrent_task.h"
#include "trace_out.h"
#include <cassert>
#include "aceMesh_utils.h"
#include "tbb/mutex.h" 

namespace AceMesh_runtime {

#ifdef ACEMESH_TIME
aceMesh_double_TLS aceMesh_task::time_tls(std::pair<double, double>(0.0, 0.0) );
aceMesh_double1_TLS aceMesh_task::time_blocking(0.0 );
#endif

#ifdef _EXEC_TIME
double aceMesh_task::exec_time = 0.0;
tbb::atomic<long long> aceMesh_task::total_task_num = tbb::atomic<long long>();
#endif


#ifdef ACEMESH_SCHEDULER_PROFILING
tbb::atomic<long long> aceMesh_task::stolen_times = tbb::atomic<long long>();

tbb::atomic<long long> aceMesh_task::sum_vert_times = tbb::atomic<long long>();
tbb::atomic<int> aceMesh_task::max_reuse_distance = tbb::atomic<int>();
int aceMesh_task::total_vert_times = 0;

void aceMesh_task::print_and_reset_reuse_statistics()
{
    //std::cout<< "sum vert times : " << aceMesh_task::sum_vert_times << std::endl;
    //std::cout<< "max reuse distance: " << aceMesh_task::max_reuse_distance << std::endl;
    aceMesh_task::total_vert_times += aceMesh_task::sum_vert_times;
    aceMesh_task::sum_vert_times = 0;
    aceMesh_task::max_reuse_distance = 0;
}

void aceMesh_task::inc_reuse_distance(int last_reuse_distance)
{
    aceMesh_task::reuse_distance += (last_reuse_distance + 1);
}

int aceMesh_task::get_reuse_distance()
{
    return reuse_distance;
}

#endif


aceMesh_task::aceMesh_task():vertical_task(NULL) , my_type(NOT_SET)
{
    successor_tasks.clear();
#ifdef DEBUG_GRAPH
    ref_count_for_check = -1;
    is_joint = 0;
#endif

#ifdef ACEMESH_SCHEDULER_PROFILING
    reuse_distance = 0;
#endif

#ifdef ACEMESH_TIME
    start_t = tbb::tick_count();  //define OK 
#endif

#ifdef AUTO_PARTITION
    group_id = -1;
#endif
#ifdef SUPPORT_PARTITION
    my_part_id = -1;
#endif
//
}

aceMesh_task::aceMesh_task(int id): vertical_task(NULL), my_type(NOT_SET)
{
    successor_tasks.clear();
#ifdef DEBUG_GRAPH
    ref_count_for_check = -1;
    is_joint = 0;
#endif

#ifdef ACEMESH_SCHEDULER_PROFILING
    reuse_distance = 0;
#endif

#ifdef ACEMESH_TIME
    start_t = tbb::tick_count();  //define OK 
#endif
#ifdef AUTO_PARTITION
    group_id = -1;
#endif
#ifdef SUPPORT_PARTITION
    my_part_id = -1;
#endif
//
}

aceMesh_task::~aceMesh_task()
{

}

void aceMesh_task::get_neighbor(int neighbor, void* src_addr, struct ptr_array *neighbor_addrs)
{
    return;
}

int  aceMesh_task::get_total_successor_sum()
{
    //std::cout<<"ref_count:"<<vertical_task->ref_count()<<std::endl;
    //std::cout<<"ref_count:"<<successor_tasks[0]->ref_count()<<std::endl;
    return vertical_task == NULL ? successor_tasks.size() : successor_tasks.size() + 1;
}

void aceMesh_task::add_end_successor(task* t)
{
	vertical_task = t;
	t->increment_ref_count();
}

int aceMesh_task::add_successor(aceMesh_task* t)
{
    this->mutex_add_successor.lock();
    if (this->over == false)
    {
        if (t == NULL)
        {
            this->mutex_add_successor.unlock();
            return 0;
        }
        for (std::vector<task *>::iterator itr = successor_tasks.begin();
             itr != successor_tasks.end(); ++itr)
        {
            if (*itr == t)
            {
                this->mutex_add_successor.unlock();
                return 0;
            }
        }
        if (vertical_task == t)
        {
            this->mutex_add_successor.unlock();
            return 0;
        }
        successor_tasks.push_back(t);
        t->increment_ref_count();
        t->edge = true;
        this->mutex_add_successor.unlock();
        return 1;
    }
    this->mutex_add_successor.unlock();
    return 0;
    //t->adjust_affinity_id(this);
}

int aceMesh_task::set_vertical_task(aceMesh_task* t)
{
    this->mutex_add_successor.lock(); 
    if(this->over==false)
    { 
    if( t == NULL ) 
	{
	    this->mutex_add_successor.unlock(); 
	    return 0;
	}	
	if(vertical_task == t)
	{
	    this->mutex_add_successor.unlock(); 
		return 0;
	}
    else {
	    for(std::vector<task* >::iterator itr = successor_tasks.begin(); 
            itr != successor_tasks.end(); ++itr)
		{
		    if(*itr == t)
			{
			    this->mutex_add_successor.unlock(); 
				return 0;
			}
		}
        if(vertical_task != NULL)
        {
	        successor_tasks.push_back(t);
	        t->increment_ref_count();
			t->edge = true;
			this->mutex_add_successor.unlock(); 
            return  1;
        }
	    vertical_task = t;
	    t->increment_ref_count();
		t->edge = true;
           this->mutex_add_successor.unlock();
           return 1;
    }
   }
	this->mutex_add_successor.unlock(); 
        return 0;
}


//add by 2015/8/1
int aceMesh_task::del_successor(aceMesh_task* t)
{   
    this->mutex_add_successor.lock();
    int ed=0;
    std::cout<<"t->ref_count1 :"<<	t->ref_count()<<std::endl;
    std::cout<<"edge1:" <<t->edge<<std::endl;	
    if(t->edge)
    {
    if(t ==NULL) 
	{
	   this->mutex_add_successor.unlock();	
	   return 0;
	}
	
    if(vertical_task ==	t)
    {    	
        vertical_task = NULL;		
        t->decrement_ref_count();
		std::cout<<"t->ref_count2-1 :"<<	t->ref_count()<<std::endl;
       this->mutex_add_successor.unlock();
       return 1;
    }	
    else
    {
       for(std::vector<task* >::iterator itr = successor_tasks.begin(); 
           itr != successor_tasks.end(); )
		{
            if(*itr == t)
            {   		
                itr = successor_tasks.erase(itr);  
                t->decrement_ref_count();
                  ++ed;
				std::cout<<"t->ref_count2-2 :"<<	t->ref_count()<<std::endl;		
            }
            else
               ++itr;
        }
        this->mutex_add_successor.unlock();
        return ed;		
    }
  }
	this->mutex_add_successor.unlock(); 
        return 0;
}

task* aceMesh_task::execute() 
{
	this->finished_lock.lock();
       //this->extra_state |= 0x40;
       this->finish=true;
       this->mutex_add_successor.lock();
	this->finished_lock.unlock();
    //std::cout << "loop_id " << this->loop_id << " task_id " << this->task_id << std::endl;
#if DEBUG_GRAPH
    print_to_internal_thread_file("loop_id, %d, task_id, %d, task_type, %d, ", this->loop_id, this->task_id, this->my_type);
#ifdef SAVE_RW_INFO
    std::ofstream* out = get_file();

    for(std::vector<addr_info>::iterator itr = rw_addrs.begin(); itr != rw_addrs.end(); ++itr)
    {
        *out << " addr:" << itr->addr << "   area_type:" << itr->area_type 
         << "  type:" << itr->rw_type << "   is_neighbor:" << itr->is_neighbor ;
    }
    *out << ", ";
#endif
    print_long_long_thread_file (tick_count_now());
#endif
#ifdef ACEMESH_TIME
    aceMesh_double_TLS::reference time_struct = time_tls.local();
    time_struct.first = time_struct.first + (tbb::tick_count::now() - start_t).seconds();
	//for MPI blocking time
	if(get_task_type()==BLOCKING_TASK)
	{
      aceMesh_double1_TLS::reference blocking_ref = time_blocking.local();
      blocking_ref = blocking_ref+ (tbb::tick_count::now() - start_t).seconds();
    }
#endif

#ifdef ACEMESH_SCHEDULER_PROFILING
    if (reuse_distance > 0)
    {
        //std::cout<< "distance :" << reuse_distance << std::endl;
        ++aceMesh_task::sum_vert_times;
        if ( reuse_distance > aceMesh_task::max_reuse_distance)
        {
            aceMesh_task::max_reuse_distance = reuse_distance;
        }
    }
    if( this->is_stolen_task() )
    {
        ++stolen_times;
    }
#endif

    unsigned int size = successor_tasks.size();
	for(unsigned int i = 0; i < size; ++i)
	{   		
		if(task* t = successor_tasks[i]) 
		{
                    t->edge=false;
		   if(t->decrement_ref_count() == 0)
		   {
		      //if( t->is_ready_task() == false)
		      if( t->ready == false)
		      {
			     this->mutex_add_successor.unlock();
				// while(t->is_ready_task() == false)
				while(t->ready == false)
				 {
				 }
				 this->mutex_add_successor.lock();
				 if(t->ref_count() == 0&&t->pre_count()!=0)
				 {
				    task::spawn(*t);
				 }
		      }
			 // if( t->is_ready_task() == true) 
			 else if( t->ready == true&&t->ref_count() == 0&&t->pre_count()!=0)
			  {
			     task::spawn(*t);
			  }
		   }	
		}		
	}

#ifdef _RETURN_VERTICAL
    if(vertical_task != NULL)
   {
       vertical_task->edge=false;
	    if(vertical_task->decrement_ref_count() == 0)
        {
#ifdef ACEMESH_SCHEDULER_PROFILING
            ((aceMesh_task*)vertical_task)->inc_reuse_distance(reuse_distance);
#endif
#ifdef ACEMESH_TIME
    time_struct.second = time_struct.second + (tbb::tick_count::now() - start_t).seconds();
#endif
		    //if( vertical_task->is_ready_task() == false)
		    if( vertical_task->ready == false)
		    {
			   this->mutex_add_successor.unlock();
			   //while(vertical_task->is_ready_task() == false)
			   while(vertical_task->ready == false)
			   {
			   }
			   this->mutex_add_successor.lock();
			//   if( vertical_task->ref_count() == 0 )
                        if( vertical_task->ref_count() == 0 &&vertical_task->pre_count()!=0)
		       {
                              this->over=true;
			    this->mutex_add_successor.unlock(); 
		        return vertical_task;
		       }
		    }
			//if(vertical_task->is_ready_task() == true) 
			else if(vertical_task->ready == true&&vertical_task->ref_count() == 0&&vertical_task->pre_count()!=0) 
                        {
                           this->over=true;
			   this->mutex_add_successor.unlock(); 
			   return vertical_task;
			}
       }
  }
#ifdef ACEMESH_TIME
    time_struct.second = time_struct.second + (tbb::tick_count::now() - start_t).seconds();
#endif
   this->over=true;
    this->mutex_add_successor.unlock(); 
	return NULL;
#else
//	this->mutex_add_successor.unlock(); 
	if(vertical_task != NULL)
	{
           vertical_task->edge=false; 
	      if(vertical_task->decrement_ref_count() == 0)
          {
#ifdef ACEMESH_SCHEDULER_PROFILING
            ((aceMesh_task*)vertical_task)->inc_reuse_distance(reuse_distance);
#endif
            //if( vertical_task->is_ready_task() == false)
            if( vertical_task->ready == false)
		    {
			   this->mutex_add_successor.unlock();
			   //while(vertical_task->is_ready_task() == false)
			   while(vertical_task->ready== false)
			   {
			   }
			   this->mutex_add_successor.lock();
			  if( vertical_task->ref_count() == 0 &&vertical_task->pre_count()!=0)
		       {
		         task::spawn(*vertical_task);
		       }
		    }
			//if(vertical_task->is_ready_task() == true) 
                       else if(vertical_task->ready == true&&vertical_task->ref_count() == 0&&vertical_task->pre_count()!=0) 
			{
			   task::spawn(*vertical_task);
			}
          }
    }
#ifdef ACEMESH_TIME
    time_struct.second = time_struct.second + (tbb::tick_count::now() - start_t).seconds();
#endif
        this->over=true;
	this->mutex_add_successor.unlock();
	return NULL;
#endif
    this->over=true;
    this->mutex_add_successor.unlock();
    return NULL;
}

#ifdef ACEMESH_TIME
void aceMesh_task::record_execute_time()
{
    this->start_t = tbb::tick_count::now();
}
void aceMesh_task::print_and_reset_execute_time()
{
    std::cout << "task pure exec time per thread," ;
    for(aceMesh_double_TLS::const_iterator iter = time_tls.begin(); iter != time_tls.end(); ++iter)
    {
        std::cout <<  (*iter).first << ",";
    }
    std::cout << std::endl;
    std::cout << "task exec time per thread, " ;
    for(aceMesh_double_TLS::const_iterator iter = time_tls.begin(); iter != time_tls.end(); ++iter)
    {
        std::cout << (*iter).second << ",";
        //iter->first = iter->second = 0.0;
    }
    std::cout << std::endl;
	double maxt=0.0;
    std::cout << "MPI blocking time per thread," ;
    for(aceMesh_double1_TLS::const_iterator iter = time_blocking.begin(); iter != time_blocking.end(); ++iter)
    {
		if(*iter>maxt) maxt=*iter;
        std::cout <<  *iter << ",";
    }
    std::cout << std::endl;
    std::cout << "max blocking time " << maxt << std::endl;

}
#endif

#ifdef PAPI_PERFORMANCE
void aceMesh_task::papi_performance_start()
{

}
void aceMesh_task::papi_performance_end()
{

}
#endif

void aceMesh_task::set_loop_id(int id)
{
    this->loop_id = id;
}

void aceMesh_task::set_task_id(int id)
{
    this->task_id = id;
}

int aceMesh_task::get_loop_id()
{
    return this->loop_id ;
}

int aceMesh_task::get_task_id()
{
    return this->task_id ;
}

#ifdef DEBUG_GRAPH
int aceMesh_task::get_ref_count_for_check()
{
    return ref_count_for_check;
}

int aceMesh_task::dec_ref_count_for_check()
{
    --ref_count_for_check;
    return ref_count_for_check;
}
void aceMesh_task::set_ref_count_for_check(int value)
{
    ref_count_for_check = value;
}

void aceMesh_task::set_joint()
{
    is_joint = 1;
}
bool aceMesh_task::is_end_task()
{
    return is_joint;
}
/*void aceMesh_task::set_loop_id(int id)
{
    this->loop_id = id;
}

void aceMesh_task::set_task_id(int id)
{
    this->task_id = id;
}

int aceMesh_task::get_loop_id()
{
    return this->loop_id ;
}

int aceMesh_task::get_task_id()
{
    return this->task_id ;
}*/


aceMesh_task* aceMesh_task::get_vertical_task()
{
    return (aceMesh_task*)vertical_task;
}
aceMesh_task* aceMesh_task::get_successor_task(int i)
{
    return (aceMesh_task*)successor_tasks[i];
}

void aceMesh_task::dfs(int& task_nums, int deep_length)
{
    if(this->is_joint == 0 && this->get_successor_sum() == 0)
    {
        std::cout<<"alone task! in deep length : "<<deep_length <<  std::endl;
        std::cout << "warining! add end to this task"<< std::endl;
        //this->set_vertical_task();
        assert(0);
    }
    //assert(0);
    --task_nums;
    if ( vertical_task != NULL )
    {
        aceMesh_task* t = (aceMesh_task *)( vertical_task );
        if(t->ref_count_for_check == -1)
        {
            t->ref_count_for_check = t->ref_count();
        }
        else 
        {
            //std::cout<<"task ref_count :"<< t->ref_count() << std::endl;
            //std::cout<<"houji task  nums:"<< t->get_successor_sum() << std::endl;
            assert(t->ref_count_for_check > 0);
        }
        --(t->ref_count_for_check);
        if(t->ref_count_for_check == 0)
        {
            t->dfs(task_nums, deep_length + 1);
        }
    }
	for(unsigned int i = 0; i < successor_tasks.size(); ++i)
	{
		if(task* tmp = successor_tasks[i])
        {
            aceMesh_task* t = (aceMesh_task *)( tmp );
            if(t->ref_count_for_check == -1)
            {
                t->ref_count_for_check = t->ref_count();
            }
            else 
            {
                //std::cout<<"task ref_count :"<< t->ref_count() << std::endl;
                assert(t->ref_count_for_check > 0);
            }
            --(t->ref_count_for_check);
            if(t->ref_count_for_check == 0)
            {
                t->dfs(task_nums, deep_length + 1);
            }
        }
    }
}

int  aceMesh_task::get_successor_sum()
{
    return successor_tasks.size() ;
}

#endif

}
