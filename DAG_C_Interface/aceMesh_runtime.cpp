#include <string>
#include <cstdarg>
#include <cassert>
#include <vector>
#include <cmath>

#include "aceMesh_runtime.h"

#if defined(DYNAMIC_SCHEDULER)
#include "aceMesh_scheduler_init.h"
#elif defined(ACEMESH_SCHEDULER)
#include "aceMesh_scheduler_init_v2.h"
#include "scheduler.h"
#endif
/*
#ifdef MEMORY_POOL
extern "C" void FreePool();
#endif
*/

#ifdef  NO_PARALLEL
#include "task_dag_graph.h"
using namespace AceMesh_runtime;
#else
#include "concurrent_task_dag_graph.h"
using namespace AceMesh_runtime;
typedef concurrent_task_dag_graph task_dag_graph; 
#endif

#include "trace_out.h"

#ifdef DATA_RACE_TRACE
#include "DataRaceTrace.h"
#endif

#define MAX_THREADS 512
using std::string;
string versionString1("305297d66e33248c6297da003b7c74cb0e0005b1 ");
string versionString2("git date: 2020-09-03 10:57:58");
string CompileTime("2021-04-18 23:25:44");

string aceMesh_outdir;
int my_mpi_rank=-1;

aceMesh_scheduler_init init;
task_dag_graph task_graph;



//runtime interface
#include <iostream>

void echo_versionString()
{
  if(my_mpi_rank>=1) 
	return;
   std::cout<<"----------------------------------------------"<<std::endl;
   std::cout<<"AceMesh Scheduler's Version Info:"<<std::endl;
   std::cout<<"Git Version Number: " << versionString1 <<std::endl;
   std::cout<<versionString2<<std::endl;
   std::cout<<"the Compile (Build) Time of this Lib is "<<CompileTime<<std::endl;
   return;

}
#ifdef __ACEMESH_THREAD_GROUP
#include "splitter.h"
// the first param is the number of dimensions, the folowing are the chunks on each dimension.
splitter cur_splitter(3, 1, 1, 1);
#endif

/**************************************************************/
/* we accept runtime environments as                          */
/* ACEMESH_NUM_THREADS, ACEMESH_THREAD_GROUP_SIZE             */
/* ACEMESH_CORE_LIST,   ACEMESH_SPLIT_CHUNKS                  */
/*runtime environments are inferior to defs inside the program*/
/* apply environment variable settings,                       */
/* if they are not covered by the application.                */
/**************************************************************/

void get_env_settings(int & total_threads, int & group_size,  std::vector<int> &vec_core_ids)

{
    char* p_acemesh_num_threads;
    char* p_acemesh_thread_group_size;
    char* p_acemesh_core_list;
    char* p_acemesh_split_chunks;

    p_acemesh_num_threads = getenv("ACEMESH_NUM_THREADS");
    if(p_acemesh_num_threads != NULL)
    {
        total_threads = atoi(p_acemesh_num_threads);
        if(total_threads < 0)
        {
           std::cerr << "wrong number of thread group: " << total_threads << std::endl;
           exit(1);
         }
    }else
	  total_threads=-1;
    p_acemesh_thread_group_size = getenv("ACEMESH_THREAD_GROUP_SIZE");
    if(p_acemesh_thread_group_size != NULL)
    {
        group_size = atoi(p_acemesh_thread_group_size);
        if(group_size < 0)
        {
            std::cerr << "wrong group size: " << group_size << std::endl;
            exit(1);
        }
	}else
	  group_size=-1;

    p_acemesh_core_list = getenv("ACEMESH_CORE_LIST");
    //std::vector<int> vec_core_ids;
    if(p_acemesh_core_list != NULL)
    {
//        std::cout << "core id env string:" << p_acemesh_core_list << std::endl;
        int i, first, last;
        bool is_range = false;
        if(*p_acemesh_core_list != '[')
        {
            std::cerr << "invalid ACEMESH_CORE_LIST argument." << std::endl;
            exit(1);
        }
        while(*p_acemesh_core_list != ']')
        {
            ++p_acemesh_core_list;
            i = atoi(p_acemesh_core_list);
            if(is_range)
            {
                last = i;
                assert(first < last);
                for (int ii = first + 1; ii <= last; ++ii)
                {
                    vec_core_ids.push_back(ii);
                }
                is_range = false;
            }
            else
            {
                vec_core_ids.push_back(i);
            }

            while(*p_acemesh_core_list != ',' && *p_acemesh_core_list != ']' && *p_acemesh_core_list != '-')
                ++p_acemesh_core_list;

            if(*p_acemesh_core_list == '-')
            {
                is_range = true;
                first = i;
            }
        }

        if(vec_core_ids.size() > MAX_THREADS)
        {
            std::cout << "core list larger than "<< MAX_THREADS << " is not currently supported." << std::endl;
            exit(1);
        }
		assert(vec_core_ids.size()>=(unsigned int)total_threads);

    }
    
#ifdef __ACEMESH_THREAD_GROUP
    p_acemesh_split_chunks = getenv("ACEMESH_SPLIT_CHUNKS");
    std::vector<size_t> dim_chunks;
    if(p_acemesh_split_chunks != NULL)
    {
        //std::cout << "split env string: " << p_acemesh_split_chunks << std::endl;        
        int i;
        if(*p_acemesh_split_chunks != '(')
        {
            std::cerr << "invalid ACEMESH_SPLIT_CHUNKS argument." << std::endl;
            return;
        }
        while(*p_acemesh_split_chunks != ')')
        {
            ++p_acemesh_split_chunks;
            //assuming single digit is enough for describing splits ,by lchen
			i = atoi(p_acemesh_split_chunks);
            dim_chunks.push_back(i);
            while(*p_acemesh_split_chunks != ',' && *p_acemesh_split_chunks != ')')
                ++p_acemesh_split_chunks;
        }
        
        cur_splitter.set_dim_chunks(dim_chunks);
		int total;
        if(group_size!=-1 && (total=cur_splitter.get_total_chunks()) != group_size)
        {
            std::cerr << "splitter dimension chunks,"<<total<<" does not match thread group size"<< group_size << std::endl;
            exit(1);
        }
        std::cout << "environment setting on split_chunks(ncuts, ... ,ncuts):" ;
		std::cout << " " << dim_chunks[0];
		for(size_t i=1;i< dim_chunks.size();i++)
           std::cout << ", " <<dim_chunks[i];
        std::cout << std::endl;
    }
#endif
}


void echo_configurations()
{
  if(my_mpi_rank>=1) 
	return;
  std::cout << "/*********************************************/" <<std::endl;
  std::cout << "Current Configurations of the Scheduler:       " <<std::endl;

/*basic scheduler*/
  std::cout << "BASIC SCHEDULER            :";
#if defined(DYNAMIC_SCHEDULER)
  std::cout << " DYNAMIC " << std::endl;
#elif defined(ACEMESH_SCHEDULER)
  std::cout << " ACEMESH_OWN " <<std::endl;
  std::cout << "DETAILED STRATEGY          : ";

  std::string details;
#ifdef USE_TBB_QUEUE
  details+="tbb_queue,";
#endif

#ifdef USE_DEQUE
  details+="queue,";
#endif
#ifdef USE_MY_OPT_STRATEGY
  details+="my_opt,";
#endif
#ifdef USE_PRIORITY_QUEUE
  details+="priority_queue,";
#endif
#ifdef USE_STACK
  details+="stack,";
#endif
//shared queue
#ifdef USE_SHARED_QUEUE
  details+=":shared_queue";
#endif
#ifdef USE_SHARED_STACK
  details+=":shared_stack";
#endif

#ifdef USE_STEALING
  details+=":stealing";
#endif
 std::cout<< details <<std::endl;

#endif

#if defined(ACEMESH_SCHEDULER)
/*if enable hierarchical execution*/
  std::cout <<"HIER_EXEC                  :";
#ifdef __ACEMESH_THREAD_GROUP
  std::cout << " ON (At runtime_init)";
#else
  std::cout << " OFF";
#endif
  std::cout <<std::endl;

  std::cout <<"CPU_BIND                   :";
#ifdef CPUBIND
  std::cout << " ON "<< std::endl;
#else
  std::cout << " OFF "<<std::endl;
#endif

  /*if enable parallel register*/
  std::cout <<"PAR_REGISTER               :";
#ifdef FOR_PARALLEL
  std::cout << " ON";
#else
  std::cout << " OFF";
#endif
  std::cout <<std::endl;
#endif

/*graph partitioning*/
  std::cout << "DAG_PARTITION              :" ;
#if defined(SUPPORT_PARTITION)
  std::cout << " SUPPORT " ;
#elif defined(AUTO_PARTITION) 
  std::cout << " AUTO " ;
#else
  std::cout << " NONE " ;
#endif
  std::cout << std::endl;

/*debug and profiling*/
  std::cout << "SCHEDULER_PROFILING        :";
#if defined( ACEMESH_SCHEDULER_PROFILING )
#if defined(DACEMESH_SCHEDULER_PROFILING_EACH_TASK)
  std::cout << " ON: EACH TASK" ;
#else
  std::cout << " ON " ;
#endif  
#else
  std::cout << " OFF " ;
#endif
  std::cout << std::endl;

  std::cout << "DEBUG_GRAPH                :";
#if defined( DEBUG_GRAPH )
  std::cout << " ON " ;
#else
  std::cout << " OFF " ;
#endif
  std::cout << std::endl;
  
  std::cout << "PAPI                       :";
#if defined(ACEMESH_PERFORMANCE )
#if defined(ACEMESH_PAPI_PERFORMANCE_TOTAL)
  std::cout << " ON: TOTAL" ;
#else
  std::cout << " ON:EACH TASK" ;
#endif
#else
  std::cout << " OFF " ;
#endif
  std::cout << std::endl;
  
  std::cout << "NOT_MPI_STRATEGY           :";
#if defined( NOT_MPI_STRATEGY )
   std::cout << " ON " ;
#else
   std::cout << " OFF" ;
   #if defined( EXTRA_THREAD )
     std::cout << ", EXTRA_THREAD" ;
   #endif
   #if defined( BLOCKING_QUEUE )
     std::cout << ", BLOCKING_QUEUE" ;
   #endif
   
#endif
     std::cout << std::endl;
  std::cout << "SPECIFY_END_TASKS          :";
#if defined( SPECIFY_END_TASKS )
       std::cout << " ON" ;
#else
       std::cout << " OFF" ;
#endif
       std::cout << std::endl;

  std::cout << "MEMORY_POOL                :";
#if defined( MEMORY_POOL )
           std::cout << " ON" ;
#else
           std::cout << " OFF" ;
#endif
           std::cout << std::endl;
    std::cout << "ALL_CYCLIC                 :";
#if defined( ALL_CYCLIC )
         std::cout << " ON" ;
#else
         std::cout << " OFF" ;
#endif
         std::cout << std::endl;




  
  std::cout << "/*********************************************/" <<std::endl;

  return;
}


#ifdef __ACEMESH_THREAD_GROUP
void set_splitter_dim_chunks( size_t dim, ... )
{
	std::vector<size_t> dim_chunks;
	va_list list;
	va_start(list, dim);
	if (!dim) return;
	for (size_t i = 0; i < dim; ++i)
	{
		dim_chunks.push_back(va_arg(list, size_t));
	}
	va_end(list);

	cur_splitter.set_dim_chunks(dim_chunks);
    std::cout << "Set split_chunks(ncuts, ... , ncuts) to:" ;
	std::cout << " " << dim_chunks[0];
    for(size_t i=1; i< dim_chunks.size(); i++)
        std::cout << ", " << dim_chunks[i];
    std::cout << std::endl;
}

Error_Code apply_runtime_init(int total_threads, int group_size, int core_ids[])
{
	float tmp=total_threads/group_size;
	int n_groups=ceil(tmp);
    std::cout<<"total_threads:"<<total_threads << ", group size :" << group_size <<std::endl;

#ifdef  CPUBIND
	std::cout<<"processor ids: ";
	for (int i=0;i<total_threads; i++)
	  std::cout<<" ,"<<core_ids[i];
	std::cout<<std::endl;
#endif
    task_graph.init(n_groups);
#ifdef  CPUBIND
    init.thread_bind(core_ids, n_groups, group_size);
#else
    init.init_thread_num(total_threads);
#endif
	echo_configurations();
	return ACEMESH_OK;
}
Error_Code aceMesh_runtime_init(int total_threads, int group_size, int core_ids[])
{
	int fake1,fake2;
	std::vector<int> core_vec;
	assert(group_size>0 && total_threads>0);
    echo_versionString();	
	float tmp=total_threads/group_size;
	int n_groups=ceil(tmp);
	get_env_settings(fake1, fake2, core_vec);
    apply_runtime_init(total_threads, group_size, core_ids);	
	return ACEMESH_OK;
}

Error_Code to_runtime_init(int total_threads, int group_size, std::vector<int> &core_vec)
{
	int core_ids[MAX_THREADS]={0};
    echo_versionString();	

    
		if(core_vec.size()==0){ //default processor binding
		  for(int i=0; i<total_threads;i++)
			core_ids[i]=i;
		}
		else
          for (int i = 0; i < core_vec.size(); ++i)
            core_ids[i] = core_vec[i];
    
	apply_runtime_init(total_threads, group_size, core_ids);
	return ACEMESH_OK;
}

Error_Code aceMesh_runtime_init(int total_threads, int group_size)
{
	int fake1, fake2;
	std::vector<int> core_vec;
	int core_ids[MAX_THREADS];
	assert(total_threads>0 && group_size>0);

    echo_versionString();	
    get_env_settings(fake1, fake2, core_vec);

	to_runtime_init(total_threads,group_size, core_vec);
	return ACEMESH_OK;
}
Error_Code aceMesh_runtime_init(int total_threads) 
{
	int fake, group_size;
	std::vector<int> core_vec;
    echo_versionString();	

    get_env_settings(fake, group_size, core_vec);
	if (group_size==-1){
	  std::cout <<"use default group_size=1"<<std::endl;
      group_size=1;
	}
    to_runtime_init(total_threads,group_size, core_vec);
	return ACEMESH_OK;
}

Error_Code aceMesh_runtime_init(int total_threads,int core_ids[])
{
	int fake, group_size;
	std::vector<int> fake_vec;
    echo_versionString();	

    get_env_settings(fake, group_size, fake_vec);
	if (group_size==-1){
	  std::cout <<"use default group_size=1"<<std::endl;
      group_size=1;
	}
    apply_runtime_init(total_threads,group_size, core_ids);
	return ACEMESH_OK;

}
Error_Code aceMesh_runtime_init()
{
	int total_threads, group_size;
	std::vector<int> core_vec;
    echo_versionString();	

    get_env_settings(total_threads, group_size, core_vec);
	if (total_threads==-1){
	  std::cout <<"use default num_threads=1"<<std::endl;
	  total_threads=1;
	}
	if (group_size==-1){
	  std::cout <<"use default group_size=1"<<std::endl;
      group_size=1;
	}
    to_runtime_init(total_threads,group_size, core_vec);
	return ACEMESH_OK;
}
#else

#define PRINT_PROC_IDS(core_ids,total_threads) \
    if(my_mpi_rank==0){\
	std::cout<<"processor_ids: ";\
	for (int i=0;i<total_threads; i++)\
	  std::cout<<" ,"<<core_ids[i];\
	std::cout<<std::endl;}

Error_Code aceMesh_runtime_init(int thread_num)
{
	int fake1,fake2,ii;
	int core_ids[MAX_THREADS]={0};
	std::vector<int> core_vec;
    echo_versionString();	
    if(my_mpi_rank==0)
	  std::cout<<"thread num :"<<thread_num<<std::endl;
    get_env_settings(fake1, fake2, core_vec);
	if(core_vec.size()==0){
#ifdef MIC
   if(0){ //no better using scatter map and binding
   //this order is no better for dag: 0,4,...,224,1,5,...225,2,6,...226,3,7,...,227
   //for 16 threads, 1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,65,69,73 also no better
   //intel omp uses the following, do not known why and don't know if perform better for dag. 
   //1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,65,69,73,77,81,85,89,93,97,101,105,109,113,117,121,125,129,133,137,141,145,149,153,157,161,165,169,173,177,181,185,189,193,197,201,205,209,213,217,221,/*shuffle*/0,
   // 2,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,66,70,74,78,82,86,90,94,98,102,106,110,114,118,122,126,130,134,138,142,146,150,154,158,162,166,170,174,178,182,186,190,194,198,202,206,210,214,218,222, /*shuffle*/225,
   //3,7,11,15,19,23,27,31,35,39,43,47,51,55,59,63,67,71,75,79,83,87,91,95,99,103,107,111,115,119,123,127,131,135,139,143,147,151,155,159,163,167,171,175,179,183,187,191,195,199,203,207,211,215,219,223, /*shuffle*/226,
   //4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124,128,132,136,140,144,148,152,156,160,164,168,172,176,180,184,188,192,196,200,204,208,212,216,220,224, /*shuffle*/227
   //
    for(ii=0;ii<thread_num;ii++){
       core_vec.push_back((ii%57)*4+ii/57);
    }
    }//bind on mic is no better
#else
#ifdef EXTRA_THREAD
		for(ii=0;ii<=thread_num;ii++)
		  core_vec.push_back(ii);
#else
		for(ii=0;ii<thread_num;ii++)
		  core_vec.push_back(ii);
#endif
#endif
    }
    task_graph.init(thread_num, aceMesh_outdir);
    init_trace_ctl(aceMesh_outdir);
#ifdef CPUBIND
    assert(thread_num<=core_vec.size());
	for (int i = 0; i < core_vec.size(); ++i)
       core_ids[i] = core_vec[i];
    PRINT_PROC_IDS(core_ids, thread_num)
	init.thread_bind(core_ids, thread_num);
#else
    init.init_thread_num(thread_num);
#endif
    echo_configurations();
    return ACEMESH_OK;
}
Error_Code aceMesh_runtime_init(int thread_num, int processors[])
{
    echo_versionString();	
    if(my_mpi_rank==0)
      std::cout<<"thread num :"<<thread_num<<std::endl;
    task_graph.init(thread_num, aceMesh_outdir);
    init_trace_ctl(aceMesh_outdir);
#ifdef CPUBIND
    PRINT_PROC_IDS(processors,thread_num)
	init.thread_bind(processors, thread_num);
#else
    init.init_thread_num(thread_num);
#endif
	echo_configurations();
    return ACEMESH_OK;
}
Error_Code aceMesh_runtime_init()
{
	int thread_num, fake2, ii;
	std::vector<int> core_vec;
	int core_ids[MAX_THREADS]={0};

    echo_versionString();	
    get_env_settings(thread_num, fake2, core_vec);
	if(core_vec.size()==0)
		for(ii=0;ii< thread_num;ii++)
		  core_vec.push_back(ii);
    if(my_mpi_rank==0){
	  if (thread_num==-1){
	    std::cout <<"use default thread_num=1"<<std::endl;
	    thread_num=1;
      }
	  else
	    std::cout<<"thread num :"<<thread_num<<std::endl;
    }
    task_graph.init(thread_num, aceMesh_outdir);
    init_trace_ctl(aceMesh_outdir);
#ifdef CPUBIND
    assert(thread_num<=core_vec.size());
	for (int i = 0; i < core_vec.size(); ++i)
       core_ids[i] = core_vec[i];
    PRINT_PROC_IDS(core_ids, thread_num)
	init.thread_bind(core_ids, thread_num);
#else
    init.init_thread_num(thread_num);
#endif

	echo_configurations();
    return ACEMESH_OK;
}
#endif

Error_Code dag_start(int dagNo, int *int_vec, int n1, double *float_vec, int n2)
{
  return task_graph.dag_start(dagNo, int_vec, n1, float_vec, n2);
}
Error_Code begin_split_task(const std::string& loop_info)
{
	task_graph.begin_split_task(loop_info);

    return ACEMESH_OK;
}
Error_Code begin_split_task(char* loop_info)
{
	std::string str = loop_info;
	return begin_split_task(str);
}
Error_Code begin_split_task()
{
#ifdef DEBUG_GRAPH
	task_graph.begin_split_task("loop_no_name");
#else
#ifdef ACEMESH_SCHEDULER_PROFILING_EACH_TASK
	task_graph.begin_split_task("loop_no_name");
#else
	task_graph.begin_split_task();
#endif
#endif
    return ACEMESH_OK;
}

Error_Code end_split_task()
{
	task_graph.end_split_task();
    return ACEMESH_OK;
}

#ifdef NO_PARALLEL
std::vector<addr_tuple> addrs;
#else
typedef tbb::enumerable_thread_specific< std::vector<addr_tuple > >  Addr_TLS_ty;
Addr_TLS_ty addrs_tls;
#endif


Error_Code register_task_end(AceMesh_runtime::aceMesh_task* t)
{
  //now we have all the addrs.
  //sort(addrs.begin(), addrs.end(), )
#ifdef NO_PARALLEL
  task_graph.register_task(t,addrs);
  addrs.clear();
#else
  task_graph.register_task(t,addrs_tls.local());
  addrs_tls.local().clear();
  //TODO:  
  //std::cerr<<"sorry that register_task_end has not been realized under parallel_register mode."<<std::endl;
  //exit(1);
#endif
  return ACEMESH_OK;
}
Error_Code register_task_datainfo(AceMesh_runtime::aceMesh_task* t, int n, ...)
{
#ifndef NO_PARALLEL
	Addr_TLS_ty::reference addrs=addrs_tls.local();
    //TODO: not work yet, should use thread local storage. by lchen
	//std::cerr<<"sorry that register_task_datainfo has not been realized under parallel_register mode."<<std::endl;
    //exit(1);
#endif

#ifdef DATA_RACE_TRACE
    __AddTask2Trace(t);
#endif
	//get addrs;
	va_list args;           
	va_start(args,n);    
	addr_tuple tmp;
    addr_tuple tmp2;
	for (int i = 0; i < n; i++) 
	{  
		tmp.addr = va_arg(args, void*);   
#ifdef DATA_RACE_TRACE
        __AddData2Trace("", (double*)tmp.addr);
#endif

        int area_type = va_arg(args, int);
        if(area_type == NORMAL || area_type == SHADE || area_type == UNSHADE)
        {
            tmp.area_type = area_type;
            tmp.neighbor = va_arg(args, int);
		    tmp.type = va_arg(args, int);
		    tmp.neighbor_type = va_arg(args, int);
		    addrs.push_back(tmp);
        } 
        else if(area_type == SHADE_AND_UNSHADE)
        {
            tmp.area_type = SHADE;
            tmp.neighbor = va_arg(args, int);
		    tmp.type = va_arg(args, int);
		    tmp.neighbor_type = va_arg(args, int);
		    addrs.push_back(tmp);
		    
            tmp2.addr = tmp.addr;
            tmp2.area_type = UNSHADE;
            tmp2.neighbor = va_arg(args, int);
		    tmp2.type = va_arg(args, int);
		    tmp2.neighbor_type = va_arg(args, int);
		    addrs.push_back(tmp2);
        }
        else
        {
            assert(0);
        }
	}
    va_end(args);
    return ACEMESH_OK;
}

Error_Code register_task(AceMesh_runtime::aceMesh_task* t, int n, ...)
{
#ifndef NO_PARALLEL
	Addr_TLS_ty::reference  addrs=addrs_tls.local();
#endif

#ifdef DATA_RACE_TRACE
    __AddTask2Trace(t);
#endif
	//get addrs;
	va_list args;           
	va_start(args,n);    
	addr_tuple tmp;
    addr_tuple tmp2;
	for (int i = 0; i < n; i++) 
	{  
		tmp.addr = va_arg(args, void*);   
#ifdef DATA_RACE_TRACE
        __AddData2Trace("", (double*)tmp.addr);
#endif

        int area_type = va_arg(args, int);
        if(area_type == NORMAL || area_type == SHADE || area_type == UNSHADE)
        {
            tmp.area_type = area_type;
            tmp.neighbor = va_arg(args, int);
		    tmp.type = va_arg(args, int);
		    tmp.neighbor_type = va_arg(args, int);
		    addrs.push_back(tmp);
        } 
        else if(area_type == SHADE_AND_UNSHADE)
        {
            tmp.area_type = SHADE;
            tmp.neighbor = va_arg(args, int);
		    tmp.type = va_arg(args, int);
		    tmp.neighbor_type = va_arg(args, int);
		    addrs.push_back(tmp);
		    
            tmp2.addr = tmp.addr;
            tmp2.area_type = UNSHADE;
            tmp2.neighbor = va_arg(args, int);
		    tmp2.type = va_arg(args, int);
		    tmp2.neighbor_type = va_arg(args, int);
		    addrs.push_back(tmp2);
        }
        else
        {
            assert(0);
        }
	}
    va_end(args);
    //sort(addrs.begin(), addrs.end(), )
	task_graph.register_task(t,addrs);
#ifdef NO_PARALLEL
    addrs.clear();
#else
    addrs_tls.local().clear();
#endif
    return ACEMESH_OK;
}

/*
void spawn()
{
	task_graph.end_construct_dag();
	task_graph.spawn();
}

void wait()
{
	task_graph.wait();
}*/

void spawn_and_wait()
{
	task_graph.spawn_and_wait();

#ifdef ACEMESH_SCHEDULER_PROFILING
    AceMesh_runtime::aceMesh_task::print_and_reset_reuse_statistics();
#endif

//#ifdef ACEMESH_TIME
//    aceMesh_task::print_and_reset_execute_time();
//#endif

#ifdef DEBUG_GRAPH
    end_file_spawn();
#endif

}

void spawn_and_wait(int print_graph)
{
#ifdef DEBUG_GRAPH
	task_graph.spawn_and_wait(print_graph);
#else
    task_graph.spawn_and_wait();
#endif

#ifdef ACEMESH_SCHEDULER_PROFILING
    AceMesh_runtime::aceMesh_task::print_and_reset_reuse_statistics();
#endif

//#ifdef ACEMESH_TIME
//    aceMesh_task::print_and_reset_execute_time();
//#endif

#ifdef DEBUG_GRAPH
    end_file_spawn();
#endif

}

#if defined(AUTO_PARTITION) || defined(SUPPORT_PARTITION)
void spawn_and_wait_sep()
{
	task_graph.spawn_and_wait_with_separation();

#ifdef ACEMESH_SCHEDULER_PROFILING
    aceMesh_task::print_and_reset_reuse_statistics();
#endif

//#ifdef ACEMESH_TIME
//    aceMesh_task::print_and_reset_execute_time();
//#endif

#ifdef DEBUG_GRAPH
    end_file_spawn();
#endif
}
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
void wait_for_all_task()
{	
	task_graph.wait_for_all_task();

#ifdef ACEMESH_SCHEDULER_PROFILING
        aceMesh_task::print_and_reset_reuse_statistics();
#endif

//#ifdef ACEMESH_TIME
//	aceMesh_task::print_and_reset_execute_time();
//#endif

#ifdef DEBUG_GRAPH
	end_file_spawn();
#endif
}
#endif


#if defined(ACEMESH_SCHEDULER)
int aceMesh_get_thread_id()
{
    unsigned int myid=generic_scheduler::theTLS.get()->get_myid();
    return myid;
}
#endif

extern "C"
void aceMesh_MPI_rank(int my_rank)
{
	my_mpi_rank=my_rank;
#ifdef DEBUG_GRAPH
    if(my_mpi_rank>=0)
    { /*make a new directory! for ouput files*/
      char  outd[4],mkdircmd[10],cmd2[10];
	  sprintf(outd, "%d" ,my_mpi_rank  );
	  sprintf(mkdircmd,"mkdir %s",outd);
	  sprintf(cmd2,"rm -fr %s",outd);
	  system(cmd2);
	  system(mkdircmd);
	  aceMesh_outdir=outd;
	  aceMesh_outdir+="/";
    }else
      aceMesh_outdir="";
#endif
	return;
}
void do_register(AceMesh_runtime::aceMesh_task* p_task, int n, va_list* p_args)
{

#ifndef NO_PARALLEL
    std::vector<addr_tuple> addrs;
#endif

#ifdef DATA_RACE_TRACE
    __AddTask2Trace(p_task);
#endif
    addrs.clear();
    //get addrs;
   
    addr_tuple tmp;
    addr_tuple tmp2;
    for (int i = 0; i < n; i++) 
    {  
        tmp.addr = va_arg(*p_args, void*);
#ifdef DATA_RACE_TRACE
        __AddData2Trace("", (double*)tmp.addr);
#endif

        int area_type = va_arg(*p_args, int);
        if(area_type == NORMAL || area_type == SHADE || area_type == UNSHADE)
        {
            tmp.area_type = area_type;
            tmp.neighbor = va_arg(*p_args, int);
            tmp.type = va_arg(*p_args, int);
            tmp.neighbor_type = va_arg(*p_args, int);
            addrs.push_back(tmp);
        } 
        else if(area_type == SHADE_AND_UNSHADE)
        {
            tmp.area_type = SHADE;
            tmp.neighbor = va_arg(*p_args, int);
            tmp.type = va_arg(*p_args, int);
            tmp.neighbor_type = va_arg(*p_args, int);
            addrs.push_back(tmp);
            
            tmp2.addr = tmp.addr;
            tmp2.area_type = UNSHADE;
            tmp2.neighbor = va_arg(*p_args, int);
            tmp2.type = va_arg(*p_args, int);
            tmp2.neighbor_type = va_arg(*p_args, int);
            addrs.push_back(tmp2);
        }
        else
        {
            assert(0);
        }
    }

    //sort(addrs.begin(), addrs.end(), )
    task_graph.register_task(p_task, addrs);
}

int get_thread_num()
{
    return init.thread_num; 
}

Error_Code aceMesh_runtime_shutdown()
{
#ifdef _EXEC_TIME
    std::cout<< "exec time : " << AceMesh_runtime::aceMesh_task::exec_time << std::endl;
    std::cout << "total task num : " << AceMesh_runtime::aceMesh_task::total_task_num << std::endl;   
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
    std::cout<< "cache miss : " << AceMesh_runtime::aceMesh_task::cache_miss<< std::endl;
#endif

#ifdef ACEMESH_SCHEDULER_PROFILING
    std::cout << "total vert times : " << AceMesh_runtime::aceMesh_task::total_vert_times << std::endl;
    std::cout << "total stolen times : " << AceMesh_runtime::aceMesh_task::stolen_times << std::endl;    
    std::cout << "vert prop : " << (double)((double)AceMesh_runtime::aceMesh_task::total_vert_times / 
                                              (double)AceMesh_runtime::aceMesh_task::total_task_num)  << std::endl;    
    AceMesh_runtime::aceMesh_task::print_and_reset_execute_time();
#endif
//free tasks(if reuse, task not free when spawn)
task_graph.free_tasks();

#ifdef MEMORY_POOL
	FreePool();
#endif

#if defined(ACEMESH_SCHEDULER)
    generic_scheduler::close_worker_thread();
#endif
    return ACEMESH_OK;
}
Error_Code aceMesh_runtime_shutdown_with_proc_id(int proc_id)
{  
#ifdef _EXEC_TIME
    if(proc_id ==0)  {
      std::cout<< "\nexec time :," << AceMesh_runtime::aceMesh_task::exec_time << ", iam= " << proc_id << std::endl;
      std::cout << "\ntotal task num :," << AceMesh_runtime::aceMesh_task::total_task_num << ", iam= " << proc_id << std::endl;
    }
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_TOTAL
    if(proc_id ==0)  {
      std::cout<< "\ncache miss :," << AceMesh_runtime::aceMesh_task::cache_miss << ", iam= " << proc_id << std::endl;
    }
#endif

#ifdef ACEMESH_SCHEDULER_PROFILING
    if(proc_id ==0)  {
      std::cout << "\ntotal vert times :," << AceMesh_runtime::aceMesh_task::total_vert_times << ", iam= " << proc_id << std::endl;
      std::cout << "\ntotal stolen times :," << AceMesh_runtime::aceMesh_task::stolen_times << ", iam= " << proc_id << std::endl;
      std::cout << "\nvert prop:," << (double)((double)AceMesh_runtime::aceMesh_task::total_vert_times / 
                                              (double)AceMesh_runtime::aceMesh_task::total_task_num)  << ", iam= " << proc_id << std::endl;
    }
    AceMesh_runtime::aceMesh_task::print_and_reset_execute_time();
#endif

#if defined(ACEMESH_SCHEDULER)
    generic_scheduler::close_worker_thread();
#endif
    return ACEMESH_OK;
}
void set_separations(std::map<void*, int>& sep_datas)
{
#ifdef SUPPORT_PARTITION
    //task_graph.set_separations(sep_datas);
#endif
}

void* save_data;
int part_id = -1;
void* aceMesh_get_data()
{
    return save_data;
}

void aceMesh_set_data(void* data)
{
    save_data = data;
}
int get_part_id()
{
    return part_id;
}

void set_part_id(int p_id)
{
    part_id = p_id;
}

