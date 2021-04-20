#include "aceMesh_task.h"
#include "trace_out.h"
#include <cassert>
#include "aceMesh_utils.h"
#include "scheduler.h"
#include "aceMesh_performance.h"
#ifdef MTEST_LIGHT
extern int* comm1;
extern int* comm2;
#endif

namespace AceMesh_runtime {

#if defined(ACEMESH_SCHEDULER_PROFILING_EACH_TASK) || defined(DEBUG_GRAPH)
   std::vector<std::string> aceMesh_task::loop_names;
#endif

#ifdef ACEMESH_TIME
aceMesh_double_TLS aceMesh_task::time_tls(std::pair<double, double>(0.0, 0.0) );
aceMesh_double1_TLS aceMesh_task::time_blocking(0.0 );
#endif

#ifdef _EXEC_TIME
double aceMesh_task::exec_time = 0.0;
tbb::atomic<long long> aceMesh_task::total_task_num = tbb::atomic<long long>();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
double aceMesh_task::cache_miss = 0;
#endif


#ifdef ACEMESH_SCHEDULER_PROFILING
tbb::atomic<long long> aceMesh_task::stolen_times = tbb::atomic<long long>();
tbb::atomic<long long> aceMesh_task::sum_vert_times = tbb::atomic<long long>();
tbb::atomic<int> aceMesh_task::max_reuse_distance = tbb::atomic<int>();
long long aceMesh_task::total_vert_times = 0;
aceMesh_long_TLS aceMesh_task::thread_total_task_num(0.0);


void aceMesh_task::print_and_reset_reuse_statistics()
{
    //std::cout<< "sum vert times : " << aceMesh_task::sum_vert_times << std::endl;
    std::cout<< "max reuse distance: " << aceMesh_task::max_reuse_distance << std::endl;
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
#ifndef NOT_MPI_STRATEGY
       suspend=0;
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
#ifndef NOT_MPI_STRATEGY
       suspend=0;
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
#ifndef SWF
void aceMesh_task::get_neighbor(int neighbor, void* src_addr, struct ptr_array *neighbor_addrs)
{
    return;
}
#else
void aceMesh_task::get_neighbor( )
{
    return;
}

#endif
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

void aceMesh_task::add_successor(aceMesh_task* t)
{
    if(t == NULL) return ;
	for(std::vector<task* >::iterator itr = successor_tasks.begin(); 
            itr != successor_tasks.end(); ++itr)
		if(*itr == t)
			return;
	if(vertical_task == t)
        return ;

	successor_tasks.push_back(t);
	t->increment_ref_count();
    //t->adjust_affinity_id(this);
}

void aceMesh_task::store_info() 
{
    if (this->get_stored() == true)
        return;

    this->store_ref_count();
    this->set_reused_flag(true);
    task* vt = this->vertical_task;
    if (vt) {
        aceMesh_task *vtask = dynamic_cast<aceMesh_task *>(vt);
        if (vtask)
        {
            vtask->store_info();
        } else if (vt->get_stored() == false){
            vt->store_ref_count();
            vt->set_reused_flag(true);
            vt->set_stored(true);
        }
    }
	for(std::vector<task* >::iterator itr = successor_tasks.begin(); 
            itr != successor_tasks.end(); ++itr)
            {
                aceMesh_task* task = dynamic_cast<aceMesh_task *>(*itr);
                if (task) {
                    task->store_info();
                }
            }
    this->set_stored(true);
}

void aceMesh_task::get_free_list(std::unordered_set<task*>& wait_free_tasks)
{
    if (this->get_stored() == false)
        return;

    wait_free_tasks.insert(this); 

    task* vt = this->vertical_task;
    if (vt) {
        aceMesh_task *vtask = dynamic_cast<aceMesh_task *>(vt);
        if (vtask)
        {
            vtask->get_free_list(wait_free_tasks);
        } else if (vt->get_stored() == true){
            wait_free_tasks.insert(vt);
            vt->set_stored(false);
        }
    }
	for(std::vector<task* >::iterator itr = successor_tasks.begin(); 
            itr != successor_tasks.end(); ++itr)
            {
                wait_free_tasks.insert(*itr);
                aceMesh_task* task = dynamic_cast<aceMesh_task *>(*itr);
                if (task) {
                    task->get_free_list(wait_free_tasks);
                }
            }
    this->set_stored(false);
}

void aceMesh_task::set_vertical_task(aceMesh_task* t)
{
    if( t == NULL ) return ;
	if(vertical_task == t)
		return;
    else {
	    for(std::vector<task* >::iterator itr = successor_tasks.begin(); 
                itr != successor_tasks.end(); ++itr)
		    if(*itr == t)
			    return;
        if(vertical_task != NULL)
        {
	        successor_tasks.push_back(t);
	        t->increment_ref_count();
            return ;
	        //successor_tasks.push_back(vertical_task);
            //std::cout << "warning replace v_task" << std::endl;
        }
	    vertical_task = t;
	    t->increment_ref_count();
        //t->adjust_affinity_id(this);
    }
}

task* aceMesh_task::execute() 
{
    if (this->get_reused_flag())
        this->restore_ref_count();
   #ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
     long long papi_end_perf_val = acemesh_papi_end_perf_cnt();
   #endif
    
   // output dynamic behavior features
  #if defined(DEBUG_GRAPH) && defined(ACEMESH_SCHEDULER_PROFILING_EACH_TASK)
  if(this->get_task_type()!=BLOCKING_TASK ||
   (this->get_task_type()==BLOCKING_TASK&& !this->get_suspend())){
    print_to_internal_thread_file("%s", loop_names[this->loop_id - 1].c_str());    
    #ifdef ACEMESH_TIME    
    print_to_internal_thread_file(", "); 
    print_long_long_thread_file ((tbb::tick_count::now() - start_t).seconds() * 1000000000);
    #endif

    #ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
      long long papi_perf_val = acemesh_papi_perf_cnt();
      print_to_internal_thread_file(", "); 
      print_long_long_thread_file (papi_perf_val );
    #endif
    
    print_to_internal_thread_file(", ");
    print_long_long_thread_file (tick_count_now());

    if (reuse_distance > 0)
    {
      print_to_internal_thread_file(", 1");      
    }
    else {
      print_to_internal_thread_file(", 0");
    }
    if( this->is_stolen_task() )
    {
        print_to_internal_thread_file(", 1");
    }
    else {
        print_to_internal_thread_file(", 0");
    }
    
    print_to_internal_thread_file("\n");
  }
  #endif

   // output static behavior features
  #if defined(DEBUG_GRAPH) && !defined(ACEMESH_SCHEDULER_PROFILING_EACH_TASK) 
#ifndef NOT_MPI_STRATEGY
  if(this->get_task_type()!=BLOCKING_TASK ||
   (this->get_task_type()==BLOCKING_TASK&& !this->get_suspend())){
#endif
    print_to_internal_thread_file("loop_id, %d, task_id, %d, task_type, %d, ", this->loop_id, this->task_id, this->my_type);
    #ifdef SAVE_RW_INFO
    std::ofstream* out = get_file();
    int neighbor_count = 0;
    for(std::vector<addr_info>::iterator itr = rw_addrs.begin(); itr != rw_addrs.end(); ++itr)
    {
        if (itr->is_neighbor) {
          neighbor_count++;
        }
        *out << " addr:" << itr->addr << "   area_type:" << itr->area_type 
         << "  type:" << itr->rw_type << "   is_neighbor:" << itr->is_neighbor ;
    }
    *out << ", ";
    *out << rw_addrs.size()  - neighbor_count << ", "; 
    *out << neighbor_count << ", "; 
    #endif

    unsigned int succ_size = successor_tasks.size();    
    print_long_long_thread_file (succ_size);
    print_to_internal_thread_file(", ");

   int max_same_addr_count = 0;
   int total_same_addr_count = 0;
   AceMesh_runtime::aceMesh_task* max_t = (AceMesh_runtime::aceMesh_task*)vertical_task;
   int vertical_count = 0;
   if(vertical_task != NULL) { 
     vertical_count = aceMesh_task::get_same_addr_count(this, (AceMesh_runtime::aceMesh_task *)vertical_task);
    }
	for(int i = 0; i < succ_size; ++i) {
	  AceMesh_runtime::aceMesh_task*  t = (AceMesh_runtime::aceMesh_task*)successor_tasks[i];
     if(t != NULL) {
       int count = aceMesh_task::get_same_addr_count(this, t);
       max_same_addr_count = count;
       total_same_addr_count += count;
       if (count >= max_same_addr_count) {
                  
         max_t = t;
         if (max_same_addr_count == vertical_count) {
           max_t = (AceMesh_runtime::aceMesh_task*)vertical_task;
         }
       }
     }
	}
    print_long_long_thread_file (total_same_addr_count);
    print_to_internal_thread_file(", ");

    print_long_long_thread_file (vertical_count);
    print_to_internal_thread_file(", ");

    if(max_t == vertical_task) {
      print_to_internal_thread_file ("1");      
    }
    else {
      print_to_internal_thread_file ("0");
    }

    print_to_internal_thread_file(",%s\n", loop_names[this->loop_id - 1].c_str());    
#ifndef NOT_MPI_STRATEGY
  }
#endif
  #endif
 
#ifdef ACEMESH_TIME
    aceMesh_double_TLS::reference time_struct = time_tls.local();
    time_struct.first = time_struct.first + (tbb::tick_count::now() - start_t).seconds();
	//for MPI blocking time
   #ifndef NOT_MPI_STRATEGY	
	if(get_task_type()==BLOCKING_TASK)
	{
      aceMesh_double1_TLS::reference blocking_ref = time_blocking.local();
      blocking_ref = blocking_ref+ (tbb::tick_count::now() - start_t).seconds();
    }
    #endif
#endif		
#ifndef NOT_MPI_STRATEGY
  #ifdef EXTRA_THREAD
    if(this->get_task_type()==BLOCKING_TASK&&
       this->get_suspend()){
#ifdef USE_PRIORITY_QUEUE
      this->set_priority_id(this->get_priority_id()+1);
#endif
#ifdef MTEST_LIGHT
      if(comm1==NULL && comm2==NULL)
	  	aceMesh_task::mpi_spawn(*this);
	  else
        aceMesh_task::mpi_polling_spawn(*this); //suspend_task goto polling_queue(high level)
#else
      aceMesh_task::mpi_spawn(*this);
#endif

#ifdef ACEMESH_TIME
    time_struct.second = time_struct.second + (tbb::tick_count::now() - start_t).seconds();
#endif
      return NULL;
    }
  #endif
  #ifdef BLOCKING_QUEUE  //add wangm

    if(this->get_task_type()==BLOCKING_TASK&&
       this->get_suspend()){
#ifdef USE_PRIORITY_QUEUE
      this->set_priority_id(this->get_priority_id()+1);
#endif
      task::suspend_spawn(*this);
#ifdef ACEMESH_TIME
    time_struct.second = time_struct.second + (tbb::tick_count::now() - start_t).seconds();
#endif
      return NULL;
    }

  #endif
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
    aceMesh_long_TLS::reference tt_nums = thread_total_task_num.local();
    tt_nums = tt_nums+1;
#endif

    unsigned int size = successor_tasks.size();
	for(unsigned int i = 0; i < size; ++i)
	{
           if(task* t = successor_tasks[i])
           {
                if(t->decrement_ref_count() == 0){

#ifdef NOT_MPI_STRATEGY
                    task::spawn(*t);
#else
  #ifdef EXTRA_THREAD
                   if(((aceMesh_task*)t)->get_task_type()!=BLOCKING_TASK)
                        task::spawn(*t);
                   else
                        task::mpi_spawn(*t);
  #endif
  #ifdef BLOCKING_QUEUE
                    task::spawn(*t);
  #endif
#endif
                   }
              }
	}

    if(vertical_task != NULL)
	   if(vertical_task->decrement_ref_count() == 0) {
        #ifdef ACEMESH_SCHEDULER_PROFILING
          ((aceMesh_task*)vertical_task)->inc_reuse_distance(reuse_distance);
        #endif 
        #ifdef _RETURN_VERTICAL
//          if(((aceMesh_task*)vertical_task)->get_task_type()!=BLOCKING_TASK)
//          { 
          #ifdef ACEMESH_TIME
            time_struct.second = time_struct.second + (tbb::tick_count::now() - start_t).seconds();
          #endif
		    return vertical_task;
//          }
//          else
//          {
//#ifdef EXTRA_THREAD
//              task::mpi_spawn(*vertical_task);
//#elif BLOCKING_QUEUE
//              task::spawn(*vertical_task);
//#endif
//          }
        #else 
        
          #ifdef NOT_MPI_STRATEGY
            task::spawn(*vertical_task);  
          #else
            #ifdef EXTRA_THREAD
              if(((aceMesh_task*)vertical_task)->get_task_type()!=BLOCKING_TASK)
                task::spawn(*vertical_task);
              else
                task::mpi_spawn(*vertical_task);
            #endif
            #ifdef BLOCKING_QUEUE
              task::spawn(*vertical_task);
            #endif 
          #endif
        #endif
      }

#ifdef ACEMESH_TIME
    time_struct.second = time_struct.second + (tbb::tick_count::now() - start_t).seconds();
#endif
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
    std::cout << "task nums per thread, " ;
    for(aceMesh_long_TLS::const_iterator iter = thread_total_task_num.begin(); iter != thread_total_task_num.end(); ++iter)
    {
        std::cout << *iter << ",";
    }
    std::cout << std::endl;


}
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
void aceMesh_task::record_papi_perf_cnt()
{
    this->perf_cnt= acemesh_papi_start_perf_cnt();
}
#endif


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
