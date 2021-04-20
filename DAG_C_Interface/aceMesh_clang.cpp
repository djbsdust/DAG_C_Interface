#include <cstdarg>
#include <cassert>
#include <vector>

#include "aceMesh_runtime.h"
#include "aceMesh_scheduler_init.h"
#include "task.h"

#ifdef MEMORY_POOL
#include "MemPool.h"
#endif

#ifdef NUMA_SUPPORT
#include <map>
#include <cmath>
#endif

#if (defined NUMA_PROFILING)||(defined NUMA_SUPPORT)
#include "hwloc.h"
#include <errno.h>    
#endif

#ifdef NUMA_PROFILING
#include "tbb/atomic.h"
#endif

using namespace AceMesh_runtime;

#ifdef  NO_PARALLEL
#include "task_dag_graph.h"
typedef std::vector<addr_tuple> VectorType;
#else
#include "concurrent_task_dag_graph.h"
typedef concurrent_task_dag_graph task_dag_graph; 
typedef tbb::enumerable_thread_specific<std::vector<addr_tuple>> VectorType; 
#endif
/*
#ifdef MEMORY_POOL
 extern  struct MemPool pool;
 extern "C" void InitPool(); 
 extern "C" void* acemesh_myalloc_aligned_4(int datasize);
 extern "C" void* acemesh_myalloc_aligned_8(int datasize);
 extern "C" void* acemesh_myalloc_aligned_16(int datasize);
 extern "C" void* acemesh_myalloc_aligned_32(int datasize);
 extern "C" void ReInitial();
#endif
*/

#ifdef NUMA_SUPPORT
struct arrDetails{
    bool isSerialTouched;
    int highest_dim;
    int highest_seg;
};
std::map<void *,arrDetails>arrRecords;

int task_count[NODENUM]={0};
#endif


#ifdef NUMA_PROFILING
typedef std::vector<void *> VectorAddr;
tbb::atomic<long long> remote_access_task_num = tbb::atomic<long long>();
tbb::atomic<long long> local_access_task_num = tbb::atomic<long long>();
tbb::atomic<long long> non_alloc_task_num = tbb::atomic<long long>();
VectorAddr _v_pure_addr_tuple;
#endif


int sche_num_threads=1;
#ifdef MTEST_LIGHT
extern int* comm1;
extern int* comm2;
extern int mkind1;
extern int mkind2;
#endif

#ifdef CONCURRENT_CONSTRUCT_GRAPH
extern "C" void acemesh_wait_for_all_task()
{
        wait_for_all_task();
}
#endif

VectorType _v_addr_tuple;
int _sepid = 0;

extern task_dag_graph task_graph;

extern "C" void* get_taskfunc_pointer(int funcno) ;

#ifdef NUMA_SUPPORT 
hwloc_topology_t topology;
//hwloc_bitmap_t set;
void hwloc_init()
{
  int err;
  /* create a topology */
  err = hwloc_topology_init(&topology);
  if (err < 0) {
    std::cout<<"failed to initialize the topology"<<std::endl;
    assert(0);
  }

  err = hwloc_topology_load(topology);
  if (err < 0) {
      std::cout<<"failed to load the topology"<<std::endl;
      hwloc_topology_destroy(topology);
      assert(0);
  }
}
void hwloc_shutdown()
{
      /* Destroy topology object. */
      hwloc_topology_destroy(topology);
}
int FastLog2(int x)
{
    float fx;
    unsigned long ix, exp;

    fx = (float)x;
    ix = *(unsigned long*)&fx;
    exp = (ix >> 23) & 0xFF;

    return exp - 127;
}


int atoi_16(char s[])  
{  
    int i = 0;  
    int n = 0;  
    for (i = 0; s[i] >= '0' && s[i] <= '9'; ++i)  
    {  
        n = 16 * n + (s[i] - '0');  
    }  
    return n;  
}  
#endif
class ci_task:public AceMesh_runtime::aceMesh_task
{
public:
  void *ci_args;
  TASK_FUNCPTR ci_task_func;

  void *ci_cxx_this_pointer;
#ifndef SWF  
  NEIGHBOR_FUNCPTR ci_get_neighbors;
#else
  TASK_FUNCPTR ci_get_neighbors;
#endif
  void *ci_neighbor_args;
  

  ci_task(TASK_FUNCPTR funcptr):ci_task_func(funcptr)
  {
    neighbor_isdefine = false;
  }
  ~ci_task() {
    if (ci_args != NULL) {
      free(ci_args);
      ci_args = NULL;
    }
  }
  void define(void* a_args, unsigned int args_size)
  {
      if (args_size >0) {
#ifdef MEMORY_POOL
        ci_args=acemesh_myalloc_aligned_16(args_size);
#else
        ci_args = malloc(args_size);
#endif
        //std::cout<<ci_args<<","<<a_args<<","<<args_size<<std::endl;
        //fflush(stdout);
        memcpy(ci_args, a_args, args_size);
        //std::cout<<"after_mmemcpy"<<std::endl;
        //fflush(stdout);
     }
      else {
        ci_args = NULL;
      }
  }
  void define_neighbors(void* cxx_this_pointer, NEIGHBOR_FUNCPTR a_get_neighbors,
                       void *a_neighbor_args) {
    ci_cxx_this_pointer = cxx_this_pointer;
    ci_get_neighbors = a_get_neighbors;
    ci_neighbor_args = a_neighbor_args;
    neighbor_isdefine = true;
  }

  task *execute()
  {
      task* &local_cur_task=cur_task.local();
      local_cur_task = (task*)this;
      
//      std::cout<<"before_ci_task_func"<<std::endl;
      ci_task_func(ci_args);
//      std::cout<<"end_ci_task_func"<<std::endl;
#ifdef NUMA_PROFILING 
//    hwloc_topology_t topology;
    hwloc_bitmap_t set;
    int err;
//    /* create a topology */
//    err = hwloc_topology_init(&topology);
//    if (err < 0) {
//        std::cout<<"failed to initialize the topology"<<std::endl;
//        assert(0);
//    }
//    err = hwloc_topology_load(topology);
//    if (err < 0) {
//        std::cout<<"failed to load the topology"<<std::endl;
//        hwloc_topology_destroy(topology);
//        assert(0);
//    }
    
      std::vector<void *> local_addr_vec_per_task=local_cur_task->get_addr_vector();
      for(int i=0;i<local_addr_vec_per_task.size();++i){
          set = hwloc_bitmap_alloc();
          char *s;
          err= hwloc_get_area_memlocation(topology, local_addr_vec_per_task[i], 1, set, HWLOC_MEMBIND_STRICT|HWLOC_MEMBIND_BYNODESET);
          hwloc_bitmap_asprintf(&s, set);
         
          //if(set!=NULL)
          //{
          //    std::cout << s <<std::endl;
          //}
          int tmp=atoi_16(&s[2]); 
          //std::cout << s <<", "<<tmp<<std::endl;
          if(strlen(s)<10)
          {
              //this tile hasn't been actually allocated memory
              non_alloc_task_num++;
          }
          else if (tmp>0) //正整型
          {
              //int tmp=atoi(&s[2]);//s[9]-'0';
              if((tmp & (tmp-1)) == 0)// n次方，未跨NUMA node
              {
                  if(FastLog2(tmp)==local_cur_task->get_affinity_id()/CORENUM)
                  {
                      ++local_access_task_num;
//                      std::cout << "local,  set:" << set << ", s:" << s << ", affinity:"<< local_cur_task->get_affinity_id() << std::endl;
                  }
                  else
                  {
                      ++remote_access_task_num;
//                      std::cout << "remote1, set:" << set << ", s:" << s << ", affinity:"<< local_cur_task->get_affinity_id() << std::endl;
                  }
              }
              else //跨NUMA node
              {
                  ++remote_access_task_num;
                  std::cout << "remote2, set:" << set << ", s:" << s << ", tmp:"<< tmp<< ", affinity:"<< local_cur_task->get_affinity_id() << std::endl;
              }
          }else{
              std:: cout << s[9] << std::endl;
              assert(0);
          }
//              if&& (((s[9]-'0') & s[9]-'1')==0))
//              { 
//              if(s[9]-'1'  == local_cur_task->get_affinity_id()/CORENUM){
//                  ++local_access_task_num;
//                  std::cout << strlen(s) <<"  local,  set:" << set << ", s:" << s << ", affinity:"<< local_cur_task->get_affinity_id() << std::endl;
//              }else{
//                  ++remote_access_task_num;
//                  std::cout << "remote, set:" << set << ", s:" << s << ", affinity:"<< local_cur_task->get_affinity_id() << std::endl;
//                  std::cout << strlen(s) <<"  ,"<< s[0]<<","<< s[1] << ", " << s[2] << " ,..." << s[9] << "...," << std::endl;
//              }
//          }
          hwloc_bitmap_free(set);
      }

//      /* Destroy topology object. */
//      hwloc_topology_destroy(topology);
#endif
      return aceMesh_task::execute();
//      std::cout<<"end_execute"<<std::endl;

  }
#ifndef SWF  
  virtual void get_neighbor(int neighbor, void* src_addr, struct ptr_array *neighbor_addrs) {
//  std::cout<<"neighbor: "<<neighbor_isdefine<<","<<ci_cxx_this_pointer<<std::endl;
    if (neighbor_isdefine) {
      if (ci_cxx_this_pointer == NULL) {
          NEIGHBOR_FUNCPTR fp = ci_get_neighbors;
          fp(neighbor_addrs,ci_neighbor_args);        
      }
      else
        assert(0);
      }
  }
#else
 virtual void get_neighbor( ) {
    if (neighbor_isdefine) {
      if (ci_cxx_this_pointer == NULL) {
          NEIGHBOR_FUNCPTR fp = ci_get_neighbors;
          fp(ci_neighbor_args);
      }
      else
        assert(0);
      }
  }
#endif
  private:
  int neighbor_isdefine;
};

namespace AceMesh_runtime {
  //expose task interface to those using acemesh_task_generator()
  tbb::enumerable_thread_specific<task*> cur_task; 
}
extern "C" void acemesh_task_set_type(int type) {
    task* &local_cur_task=cur_task.local();
    ((aceMesh_task*)local_cur_task)->set_task_type(type);
}
extern "C" void acemesh_task_set_type_(int type) {
    acemesh_task_set_type(type);
}
extern "C" void acemesh_task_set_affinity(int id) {
   task* &local_cur_task=cur_task.local();
   ((aceMesh_task*)local_cur_task)->set_affinity_id(id);
}
extern "C" void acemesh_task_set_affinity_(int id){
   acemesh_task_set_affinity(id);
}
//#ifdef USE_PRIORITY_QUEUE
extern "C" void acemesh_task_set_priority_id(int id) {
   task* &local_cur_task=cur_task.local();
   ((aceMesh_task*)local_cur_task)->set_priority_id(id);
}
extern "C" void acemesh_task_set_priority_id_(int id){
   acemesh_task_set_priority_id(id);
}

extern "C" int acemesh_task_get_priority_id() {
   task* &local_cur_task=cur_task.local();
   return (((aceMesh_task*)local_cur_task)->get_affinity_id());
}
extern "C" int acemesh_task_get_priority_id_() {
   return acemesh_task_get_priority_id();
}

//#endif

extern "C" void acemesh_set_suspend(int mode) {
#ifndef NOT_MPI_STRATEGY
  task* &local_cur_task=cur_task.local();
  ((aceMesh_task*)local_cur_task)->set_suspend(mode);
#endif
}
extern "C" void acemesh_set_suspend_(int mode) {
  acemesh_set_suspend(mode);
}

#ifdef MTEST_LIGHT
/*extern "C" void acemesh_set_event(int *comm_hand1,int *comm_hand2){
    comm1=comm_hand1;
    comm2=comm_hand2;
}
extern "C" void acemesh_set_event_(int *comm_hand1,int *comm_hand2){
    acemesh_set_event( comm_hand1,comm_hand2);
}*/
extern "C" void acemesh_set_mtest_handle(int *comm_hand1,int *comm_hand2,int kind1,int kind2){
    comm1=comm_hand1;
    comm2=comm_hand2;
    mkind1=kind1;
    mkind2=kind2;
}
extern "C" void acemesh_set_mtest_handle_(int *comm_hand1,int *comm_hand2,int kind1,int kind2){
    acemesh_set_mtest_handle( comm_hand1,comm_hand2,kind1,kind2);
}
#endif

extern "C" void acemesh_runtime_init(int total_threads) {
#ifdef NUMA_SUPPORT 
  hwloc_init();
#endif

  if(total_threads == 0)
    aceMesh_runtime_init();
  else
    aceMesh_runtime_init(total_threads);
#ifdef ACEMESH_PERFORMANCE
    acemesh_papi_init();
#endif
#ifdef MEMORY_POOL
    InitPool();
#endif

}
extern "C" void acemesh_runtime_init_(int total_threads) {
  acemesh_runtime_init(total_threads);
}
extern "C" void acemesh_runtime_shutdown() {
#ifdef ACEMESH_PERFORMANCE
    acemesh_papi_uninit();
#endif
#ifdef NUMA_SUPPORT
    hwloc_shutdown();
#ifdef NUMA_PROFILING
    double local_access_ratio=(double)local_access_task_num/(double)(local_access_task_num+remote_access_task_num);
    std::cout<<"total task num: "<<aceMesh_task::total_task_num << std::endl;
    std::cout<<"remote_access_task_num:"<<remote_access_task_num<<", local_access_task_num:"<<local_access_task_num<< ", non_alloc_task_num:"<< non_alloc_task_num<<std::endl;
    std::cout<<"LOCAL ACCESS RETIO: "<<local_access_ratio<<std::endl;
#endif
#endif

  aceMesh_runtime_shutdown();
}
extern "C" void acemesh_runtime_shutdown_() {
  acemesh_runtime_shutdown();
}
extern "C" void acemesh_runtime_shutdown_with_proc_id(int proc_id) {
#ifdef ACEMESH_PERFORMANCE
    acemesh_papi_uninit();
#endif
  aceMesh_runtime_shutdown_with_proc_id(proc_id);
}
extern "C" void acemesh_runtime_shutdown_with_proc_id_(int proc_id) {
  acemesh_runtime_shutdown_with_proc_id(proc_id);
}

#ifdef SPECIFY_END_TASKS
extern "C" void acemesh_specify_end_tasks()
{
    task* &local_cur_task=cur_task.local();
    task_graph.acemesh_add_end_task((aceMesh_task*)local_cur_task);
}

extern "C" void acemesh_specify_end_tasks_(){
  acemesh_specify_end_tasks();
}


#endif


extern "C" void 
acemesh_task_generator(TASK_FUNCPTR taskfptr,  void* args,  unsigned int args_size)
{
  ci_task* _task = ACEMESH_NEW ci_task(taskfptr);
  _task->define(args, args_size);

  //printf("acemesh_task_generator2\n");
   task* &local_cur_task=cur_task.local();
    local_cur_task = _task;

#ifdef SUPPORT_PARTITION
    _task->set_part_id(_sepid);
#endif

#ifdef NO_PARALLEL
  std::vector<addr_tuple>& local_vector = _v_addr_tuple;
#ifdef NUMA_PROFILING 
  std::vector<void *>& local_addr_vector = _v_pure_addr_tuple;
#endif
#else
  std::vector<addr_tuple>& local_vector = _v_addr_tuple.local();
#endif

  task_graph.register_task(_task, local_vector);
  //printf("acemesh_task_generator3\n");
  local_vector.clear();  
#ifdef NUMA_PROFILING
  local_cur_task->set_addr_vector(local_addr_vector);
  local_addr_vector.clear();
#endif
  //std::cout<<"after_register"<<std::endl;
}
extern "C" void 
acemesh_task_generator_(TASK_FUNCPTR taskfptr,  void* args,  unsigned int args_size)
{
    acemesh_task_generator(taskfptr,args,args_size);
}
extern "C" void 
acemesh_task_generator_swf(int task_funcno,  void* args,  unsigned int args_size) {
  //acemesh_task_generator(get_taskfunc_pointer(task_funcno), args, args_size);
}
extern "C" void 
acemesh_task_generator_swf_(int task_funcno,  void* args,  unsigned int args_size) {
  //acemesh_task_generator(get_taskfunc_pointer(task_funcno), args, args_size);
}
extern "C" void
acemesh_task_generator_swf2(void* taskfptr,  void* args,  unsigned int args_size) {
  acemesh_task_generator(taskfptr, args, args_size);
}
extern "C" void
acemesh_task_generator_swf2_(void* taskfptr,  void* args,  unsigned int args_size) {
  acemesh_task_generator(taskfptr, args, args_size);
}

extern "C" void
acemesh_task_generator_with_neighbors(TASK_FUNCPTR taskfptr,  void* args,  unsigned int args_size,
  void *cxx_this_pointer, NEIGHBOR_FUNCPTR get_neighbors_funcptr, void *neighbor_args) {
  ci_task* _task = ACEMESH_NEW ci_task(taskfptr);
  _task->define(args, args_size);
  _task->define_neighbors(cxx_this_pointer, get_neighbors_funcptr, neighbor_args);

  task* &local_cur_task=cur_task.local();
  local_cur_task = _task;

#ifdef SUPPORT_PARTITION
    _task->set_part_id(_sepid);
#endif

  addr_tuple item;
  item.addr = NULL;
  item.area_type = NORMAL;  //!!NOTE, same as UNSHADE, see comments in get_unshade_addr(),by chenli 
  item.type = INOUT_NONE;
  item.neighbor = NEIGHBOR_ALL;
  item.neighbor_type = IN;

#ifdef NO_PARALLEL
  std::vector<addr_tuple>& local_vector = _v_addr_tuple;
#ifdef NUMA_PROFILING 
  std::vector<void *>& local_addr_vector = _v_pure_addr_tuple;
#endif
#else
  std::vector<addr_tuple>& local_vector = _v_addr_tuple.local();
#endif

#ifdef NUMA_PROFILING
  local_cur_task->set_addr_vector(local_addr_vector);
  local_addr_vector.clear();
#endif  
  local_vector.push_back(item);
  
  task_graph.register_task(_task,local_vector);
  local_vector.clear();
  //std::cout<<"after_neigh_register"<<std::endl;
  //fflush(stdout);
}

extern "C" void
acemesh_task_generator_with_neighbors_(TASK_FUNCPTR taskfptr,  void* args,  unsigned int args_size,
  void *cxx_this_pointer, NEIGHBOR_FUNCPTR get_neighbors_funcptr, void *neighbor_args) {
  acemesh_task_generator_with_neighbors(taskfptr,  args,  args_size,  cxx_this_pointer, get_neighbors_funcptr, neighbor_args);
}
extern "C" void
acemesh_task_generator_with_neighbors_swf2(void* task_funcptr,  void* args,  unsigned int args_size,
        void *cxx_this_pointer, void *neighbor_task_funcptr, void *neighbor_args) {
        acemesh_task_generator_with_neighbors(task_funcptr, args, args_size, cxx_this_pointer,
        neighbor_task_funcptr, neighbor_args);
}
extern "C" void
acemesh_task_generator_with_neighbors_swf2_(void* task_funcptr,  void* args,  unsigned int args_size,
        void *cxx_this_pointer, void *neighbor_task_funcptr, void *neighbor_args) {
        acemesh_task_generator_with_neighbors(task_funcptr, args, args_size, cxx_this_pointer,
        neighbor_task_funcptr, neighbor_args);
}

static void 
acemesh_do_push(int argc, void *addr, int area_flag, va_list* p_args, int type)
{
  addr_tuple item;

  item.type = type;    
  item.neighbor = NEIGHBOR_NONE;
  item.neighbor_type = INOUT_NONE;

  item.addr = addr;
  item.area_type = area_flag;

#ifdef NO_PARALLEL
  std::vector<addr_tuple>& local_vector = _v_addr_tuple;
#ifdef NUMA_PROFILING  
  std::vector<void *>& local_addr_vector = _v_pure_addr_tuple;
#endif
#else
  std::vector<addr_tuple>& local_vector = _v_addr_tuple.local();
#endif


  if(area_flag == SHADE_AND_UNSHADE) {
    item.area_type= SHADE;    
    local_vector.push_back(item);

    item.area_type = UNSHADE;
    local_vector.push_back(item);
  }
  else {
    local_vector.push_back(item);
  }
#ifdef NUMA_PROFILING
  local_addr_vector.push_back(addr);
#endif
  while(argc-1) {
    item.addr = va_arg(*p_args, void*);
    item.area_type = va_arg(*p_args, int);
    if(item.area_type == SHADE_AND_UNSHADE){
      item.area_type= SHADE;    
      local_vector.push_back(item);

      item.area_type = UNSHADE;
      local_vector.push_back(item);
    }
    else{
      local_vector.push_back(item);
    }
#ifdef NUMA_PROFILING
    local_addr_vector.push_back(item.addr);
#endif
    --argc;
  }
}
static void 
acemesh_do_push1(int argc, void *addr, int area_flag,  int type) {
  addr_tuple item;

  item.type = type;    
  item.neighbor = NEIGHBOR_NONE;
  item.neighbor_type = INOUT_NONE;

  item.addr = addr;
  item.area_type = area_flag;

#ifdef NO_PARALLEL
  std::vector<addr_tuple>& local_vector = _v_addr_tuple;
#ifdef NUMA_PROFILING
  std::vector<void *>& local_addr_vector = _v_pure_addr_tuple;
  local_addr_vector.push_back(addr);
#endif
#else
  std::vector<addr_tuple>& local_vector = _v_addr_tuple.local();
#endif

  if(area_flag == SHADE_AND_UNSHADE)
  {
    item.area_type= SHADE;    
    local_vector.push_back(item);

    item.area_type = UNSHADE;
    local_vector.push_back(item);
  }
  else {
    local_vector.push_back(item);
  }

}

extern "C" void acemesh_push_wrlist(int argc, void *addr, int area_flag,...) {
  va_list args;
  va_start(args, area_flag);
  acemesh_do_push(argc, addr, area_flag, &args, INOUT);
  va_end(args);
}
extern "C" void acemesh_push_wrlist1(int argc, void *addr, int area_flag) {
  acemesh_do_push1(argc, addr, area_flag, INOUT);
}
extern "C" void acemesh_push_wrlist1_(int argc, void *addr, int area_flag) {
  acemesh_do_push1(argc, addr, area_flag, INOUT);
}

extern "C" void acemesh_push_rlist(int argc, void *addr, int area_flag,...) {
  va_list args;
  va_start(args, area_flag);
  acemesh_do_push(argc, addr, area_flag, &args, IN);
  va_end(args);
}
extern "C" void acemesh_push_rlist1(int argc, void *addr, int area_flag) {
  acemesh_do_push1(argc, addr, area_flag, IN);
}
extern "C" void acemesh_push_rlist1_(int argc, void *addr, int area_flag) {
  acemesh_do_push1(argc, addr, area_flag, IN);
}

extern "C" void acemesh_push_wlist(int argc, void *addr, int area_flag,...){
  va_list args;
  va_start(args, area_flag);
  acemesh_do_push(argc, addr, area_flag, &args, OUT);
  va_end(args);
}
extern "C" void acemesh_push_wlist1(int argc, void *addr, int area_flag)  {
  acemesh_do_push1(argc, addr, area_flag, OUT);
}
extern "C" void acemesh_push_wlist1_(int argc, void *addr, int area_flag)  {
  acemesh_do_push1(argc, addr, area_flag, OUT);
}

#ifdef SWF 
extern "C" void acemesh_push_neighbor_addr(void *addr) {
  task_graph.push_neighbor_addr(addr);
}
extern "C" void acemesh_push_neighbor_addr_(void *addr) {
  acemesh_push_neighbor_addr(addr);
}
#endif
extern "C" void acemesh_begin_split_task(char * loop_info) {
  if((loop_info == 0) || (*loop_info == '\0'))
    begin_split_task("loop_no_name");
  else
    begin_split_task(loop_info);
}
extern "C" void acemesh_begin_split_task_(char * loop_info) {
  acemesh_begin_split_task(loop_info);
}
extern "C" void acemesh_end_split_task() {
#ifdef NUMA_SUPPORT
  for(int i=0;i<NODENUM;++i){
    task_count[i]=0;
  }
#endif
  end_split_task();
}
extern "C" void acemesh_end_split_task_() {
#ifdef NUMA_SUPPORT
  for(int i=0;i<NODENUM;++i){
    task_count[i]=0;
  }
#endif
  end_split_task();
}
extern "C" void acemesh_task_map_master()
{
    
}

extern "C" void acemesh_task_map_master_()
{
    acemesh_task_map_master();
}

extern "C" void acemesh_reset_affinity()
{
   task_graph.reset_affinity();
}
extern "C" void acemesh_reset_affinity_()
{
   acemesh_reset_affinity();
}


static std::vector<void *> enlonged_local_objs; 
extern "C" void *acemesh_malloc_obj(unsigned int args_size) {
  void *addr = malloc(args_size);
  enlonged_local_objs.push_back(addr);
  return addr;
}
static void acemesh_free_all_objs(void) {
  std::vector<void*>::iterator itr;
  for (itr = enlonged_local_objs.begin(); 
      itr!= enlonged_local_objs.end(); 
      ++itr){
    void* addr = *itr;
    free(addr);
  } 
  enlonged_local_objs.clear();    
}
extern "C" void acemesh_spawn_and_wait(int print_graph) {
  if (print_graph == 0)
    spawn_and_wait();
  else
    spawn_and_wait(print_graph);
  acemesh_free_all_objs();
#ifdef MEMORY_POOL   
      ReInitial();
#endif

}
extern "C" void acemesh_spawn_and_wait_(int print_graph) {
  acemesh_spawn_and_wait(print_graph);
}
#ifdef SUPPORT_PARTITION
void acemesh_setsepid(int id)
{
    _sepid=id;
}
#endif
//add new,tsl
extern "C" int acemesh_get_thread_id() {
 return aceMesh_get_thread_id();
}
extern "C" int acemesh_get_thread_id_() {
 return aceMesh_get_thread_id();
}
extern "C" void acemesh_mpi_rank(int rank){
    aceMesh_MPI_rank(rank);
}
extern "C" void acemesh_mpi_rank_(int rank){
    aceMesh_MPI_rank(rank);
}

extern "C" int acemesh_get_loop_tile_num(int loop_tile_size, int loop_index_lb, int loop_index_ub) {
  int loop_tile_num;
  int loop_index_len;
  int lb_mod;
  int ub_mod;


   loop_tile_num = 0;
   if((loop_index_ub - loop_index_lb>=0&&loop_tile_size>0)||(loop_index_ub - loop_index_lb<=0&&loop_tile_size<0)){
     if(loop_index_ub - loop_index_lb>=0)
       loop_index_len = loop_index_ub - loop_index_lb + 1;
     else
     {
       loop_index_len = loop_index_ub - loop_index_lb - 1;
       
       loop_index_len=loop_index_len*(-1);
       loop_tile_size=loop_tile_size*(-1);
     }
  //pre loop
    lb_mod = loop_index_lb % loop_tile_size;
    if (lb_mod != 0) {
      loop_tile_num = loop_tile_num + 1;
    if (loop_index_len < (loop_tile_size -lb_mod)) 
       loop_index_len = 0;
     else
       loop_index_len = loop_index_len - (loop_tile_size - lb_mod);
    }

  //post loop
    ub_mod = loop_index_ub % loop_tile_size;
    if ((ub_mod != (loop_tile_size - 1)) && 
      (loop_index_len > loop_tile_size)) {
     loop_tile_num = loop_tile_num + 1;
     loop_index_len = loop_index_len - (ub_mod + 1);
    }
  //main loop
    if (loop_index_len > 0) {
      if (loop_index_len < loop_tile_size)
        loop_tile_num = loop_tile_num + 1;
      else
        loop_tile_num = loop_tile_num + loop_index_len/loop_tile_size;
    }
  }
  return loop_tile_num;

}

extern "C" int acemesh_get_loop_tile_num_(int loop_tile_size, int loop_index_lb, int loop_index_ub) {
  return acemesh_get_loop_tile_num(loop_tile_size, loop_index_lb, loop_index_ub);
}

extern "C" int acemesh_get_loop_tile_start_index_by_tile_no(int loop_tile_size, int loop_tile_no, int loop_index_lb) {
  int ret = loop_tile_no*loop_tile_size + loop_index_lb - (loop_index_lb % loop_tile_size);
  if (ret < loop_index_lb)
    return loop_index_lb;
  else
    return ret;
}
extern "C" int acemesh_get_loop_tile_start_index_by_tile_no_(int loop_tile_size, int loop_tile_no, int loop_index_lb) {

  return acemesh_get_loop_tile_start_index_by_tile_no(loop_tile_size, loop_tile_no, loop_index_lb);
}
extern "C" int acemesh_get_tile_no_by_index(int tile_size, int compute_index, int index_lb) {
    int tile_no;
    tile_no = (compute_index - (index_lb - index_lb % tile_size) ) / tile_size ;
    return tile_no;
}
extern "C" int acemesh_get_tile_no_by_index_(int tile_size, int compute_index, int index_lb) {
    return acemesh_get_tile_no_by_index(tile_size,compute_index,index_lb);
}
extern "C" int acemesh_get_loop_tile_end_index_by_tile_no(int loop_tile_size, int loop_tile_no, int loop_index_lb, int loop_index_ub) {
  int ret;
  ret = (loop_tile_no+1)* loop_tile_size + loop_index_lb - (loop_index_lb % loop_tile_size)-1;
  if (ret >loop_index_ub) 
    return loop_index_ub;
  else
    return ret;  
}
extern "C" int acemesh_get_loop_tile_end_index_by_tile_no_(int loop_tile_size, int loop_tile_no, int loop_index_lb, int loop_index_ub) {
  return acemesh_get_loop_tile_end_index_by_tile_no(loop_tile_size, loop_tile_no, loop_index_lb, loop_index_ub);
}
extern "C" int acemesh_get_data_tile_start_index_by_index(int data_tile_size, int compute_index, int array_dim_lb) {
  int ret;
  ret = compute_index - (compute_index % data_tile_size);
  if (ret < array_dim_lb)
    return array_dim_lb;
  else 
    return ret;   
}
extern "C" int acemesh_get_loop_tile_num_with_init_offset(int loop_tile_size, int loop_index_lb, int loop_index_ub, int init_offset) {
  int loop_tile_num=0;
  int lno_1,lno_2;
  if(loop_index_lb>loop_index_ub)
      return 0;
  //if((loop_index_lb<loop_index_ub && loop_tile_size > 0)||(loop_index_lb>loop_index_ub && loop_tile_size < 0))
  //{
    lno_1 = (loop_index_ub - init_offset) / loop_tile_size;
    lno_2 = (loop_index_lb - init_offset) / loop_tile_size;

    loop_tile_num = lno_1 -lno_2 +1;
  //}
 // std::cout << loop_tile_num << loop_tile_size << loop_index_lb << loop_index_ub << init_offset << lno_1 << lno_2 << std::endl;
  return loop_tile_num;

}
extern "C" int acemesh_get_loop_tile_start_index_by_tile_no_with_init_offset(int loop_tile_size, int loop_tile_no, int loop_index_lb, int init_offset) {
  int ret = (loop_tile_no +(loop_index_lb-init_offset)/loop_tile_size)* loop_tile_size + init_offset;
  //std::cout <<  loop_tile_size << loop_tile_no << loop_index_lb << init_offset << ret << std::endl;
  if (ret < loop_index_lb)
    return loop_index_lb;
  else
    return ret;
}
extern "C" int acemesh_get_loop_tile_end_index_by_tile_no_with_init_offset(int loop_tile_size, int loop_tile_no, int loop_index_lb, int loop_index_ub, int init_offset) {
  int ret;
  ret = (loop_tile_no +(loop_index_lb-init_offset)/loop_tile_size+1)* loop_tile_size + init_offset - 1;
  //ret = (loop_tile_no+1)* loop_tile_size - init_offset % loop_tile_size + loop_index_lb - (loop_index_lb % loop_tile_size)-1;
  if (ret >loop_index_ub) 
    return loop_index_ub;
  else
    return ret;  
}
extern "C" int acemesh_get_data_tile_start_index_by_index_with_init_offset(int data_tile_size, int compute_index, int array_dim_lb, int init_offset) {
  int ret;
  ret = (compute_index-init_offset)/data_tile_size*data_tile_size+init_offset;
//  ret = compute_index - ( (compute_index -init_offset % data_tile_size)% data_tile_size);
  if (ret < array_dim_lb)
    return array_dim_lb;
  else 
    return ret;   
}
extern "C" int acemesh_get_tile_no_by_index_with_init_offset(int tile_size, int index, int lb, int init_offset){
  int lno_1, lno_2, ret;
  lno_1 = (index - init_offset % tile_size) / tile_size;
  lno_2 = (lb - init_offset % tile_size) / tile_size;
  ret = lno_1 - lno_2 ;
  return ret;

} 

extern "C" int acemesh_get_data_tile_start_index_by_index_(int tile_size, int compute_index, int array_dim_lb) {
  return acemesh_get_data_tile_start_index_by_index(tile_size, compute_index, array_dim_lb);
}
extern "C" int acemesh_get_loop_tile_num_with_init_offset_(int loop_tile_size, int loop_index_lb, int loop_index_ub, int init_offset) {
  return acemesh_get_loop_tile_num_with_init_offset(loop_tile_size, loop_index_lb, loop_index_ub, init_offset);
}
extern "C" int acemesh_get_loop_tile_start_index_by_tile_no_with_init_offset_(int loop_tile_size, int loop_tile_no, int loop_index_lb, int init_offset) {

  return acemesh_get_loop_tile_start_index_by_tile_no_with_init_offset(loop_tile_size, loop_tile_no, loop_index_lb, init_offset);
}
extern "C" int acemesh_get_loop_tile_end_index_by_tile_no_with_init_offset_(int loop_tile_size, int loop_tile_no, int loop_index_lb, int loop_index_ub, int init_offset) {
  return acemesh_get_loop_tile_end_index_by_tile_no_with_init_offset(loop_tile_size, loop_tile_no, loop_index_lb, loop_index_ub, init_offset);
}
extern "C" int acemesh_get_data_tile_start_index_by_index_with_init_offset_(int tile_size, int compute_index, int array_dim_lb, int init_offset) {
  return acemesh_get_data_tile_start_index_by_index_with_init_offset(tile_size, compute_index, array_dim_lb, init_offset);
}
extern "C" int acemesh_get_tile_no_by_index_with_init_offset_(int tile_size, int index, int lb, int init_offset){
    return acemesh_get_tile_no_by_index_with_init_offset(tile_size, index, lb, init_offset);
}
extern "C" int slave_acemesh_athread_get(int mode, void *src, void *dest, int len, int *reply, int mask, int stride, int bsize) {
  int size_count = 0;
  char *psrc = (char *)src;
  char *pdest = (char *)dest;  
  //std::cout << "get start len=" << len << " stride=" << stride << " bsize=" << bsize << std::endl;
  if(bsize == 0) {
    bsize = len;
  }
  while(size_count < len) {
    memcpy(pdest, psrc, bsize);
    psrc += (bsize + stride);
    pdest += bsize;
    size_count += bsize;
  }
  int reply_val = *reply;
  *reply = ++reply_val;
  //std::cout << "get end len=" << len << " stride=" << stride << " bsize=" << bsize << std::endl;
  return 0;  
}
extern "C" int slave_acemesh_athread_get_(int mode, void *src, void *dest, int len, int *reply, int mask, int stride, int bsize) {  
  return slave_acemesh_athread_get(mode, src, dest, len, reply, mask, stride, bsize);
}
extern "C" int slave_acemesh_athread_put(int mode, void *src, void *dest, int len, int *reply, int stride, int bsize) {
  int size_count = 0;
  char *psrc = (char *)src;
  char *pdest = (char *)dest;
  //std::cout<<"psrc"<<*psrc<<"pdest"<<*pdest<<std::endl; 
  // printf("start:%p,%p\n",psrc,pdest);
//  std::cout << "put start len=" << len << " stride=" << stride << " bsize=" << bsize << std::endl;
  
  if(bsize == 0) {
    bsize = len;
  }
  while(size_count < len) {
    memcpy(pdest, psrc, bsize);
    psrc +=  bsize;
    pdest += (stride + bsize);
    size_count += bsize;
  }
  int reply_val = *reply;
  *reply = ++reply_val;
   //printf("end:%p,%p\n",psrc,pdest);
  //std::cout << "put end len=" << len << " stride=" << stride << " bsize=" << bsize << std::endl;
  return 0;
}

extern "C" int  slave_acemesh_athread_put_(int mode, void *src, void *dest, int len, int *reply, int stride, int bsize) {
  return slave_acemesh_athread_put(mode, src, dest, len, reply, stride, bsize);
}

extern "C" int slave_acemesh_wait_reply(int *reply, int count) {
  while(1) {
    volatile int reply_val;
    reply_val = *reply;
    if (reply_val == count) {
      return 0;
    }
  }
  return 0;
}
extern "C" int slave_acemesh_wait_reply_(int *reply, int count) {
  return slave_acemesh_wait_reply(reply, count);
}


extern "C" void acemesh_set_thread_num(int thread_num){
    sche_num_threads = thread_num;
}
extern "C" void acemesh_set_thread_num_(int thread_num){
  acemesh_set_thread_num(thread_num);
}

extern "C" int acemesh_dag_start_vec(int dagNo, int *int_vec, int n1, double *float_vec, int n2) {
  return dag_start(dagNo, int_vec, n1, float_vec, n2);
}
extern "C" int acemesh_dag_start(int dagNo) {
  return acemesh_dag_start_vec(dagNo, 0, 0, 0, 0);
}

extern "C" void slave_acemesh_reshape_copyin2d(void *data,int d1,int t1,int d2,int t2,int e_type_size, int stride_no) {
	int i,j;
	char *pdata=(char *)data;
	for(i=d2-1;i>=0;i--)
		for(j=d1-1;j>=0;j--)
			memcpy(&pdata[(i*d1+j)*e_type_size],&pdata[(i*t1+j)*e_type_size],e_type_size);
}   
	
extern "C" void slave_acemesh_reshape_copyout2d(void *data,int d1,int t1,int d2,int t2,int e_type_size, int stride_no) {
	int i,j;
	char *pdata=(char *)data;
	for(i=0;i<=t2-1;i++)
		for(j=0;j<=t1-1;j++)
			memcpy(&pdata[(i*t1+j)*e_type_size],&pdata[(i*d1+j)*e_type_size],e_type_size);
} 
/*from the 3d c array(t3,t2,t1) to the  3d c array(d3,d2,d1) */
/* the corresponding fortran array is (t1,t2,t3) and (d1,d2,d3) respectively */
/* in fact, expand data */
// stride_no starts from 0, it must be >= 1 && <= dim_num - 1
// so,in this case, stride_no is 1 or 2
extern "C" void slave_acemesh_reshape_copyin3d(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int e_type_size, int stride_no) {
	int i,j,k;
	char *pdata=(char *)data;
	
	int dim_size[3];
	int tile_size[3];
	int stride[3];
	int stride_dim;
	
	dim_size[0] = d1;
	dim_size[1] = d2;
	dim_size[2] = d3;
	tile_size[0] = t1;
	tile_size[1] = t2;
	tile_size[2] = t3;
	
	for (i=2; i > 0; i--) {
		stride[i] = 1;
		for (j = i-1; j >= 0; j--) {
			if (i > stride_no) {
				stride_dim = dim_size[j];
			}
			else {
				stride_dim = tile_size[j];
			}

			stride[i] *= stride_dim;	
		}
	}
    stride[0] = 1;
	
	for(i=d3-1;i>=0;i--)
		for(j=d2-1;j>=0;j--)
			for(k=d1-1;k>=0;k--)
				memcpy(&pdata[(i*(d2*d1)+j*d1+k)*e_type_size],
				       &pdata[(i*stride[2]+
				               j*stride[1]+k)*e_type_size],
				       e_type_size);

			
}
/*from the 3d c array(d3,d2,d1) to the  3d c array(t3,t3,t1) */
/* the corresponding fortran array is (t1,t2,t3) and (d1,d2,d3) respectively */
/* in fact, compress data first before output from ldm to mm */

extern "C" void slave_acemesh_reshape_copyout3d(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int e_type_size, int stride_no) {
	int i,j,k;
	char *pdata=(char *)data;
	int dim_size[3];
	int tile_size[3];
	int stride[3];
	int stride_dim;
	
	dim_size[0] = d1;
	dim_size[1] = d2;
	dim_size[2] = d3;
	tile_size[0] = t1;
	tile_size[1] = t2;
	tile_size[2] = t3;
    for (i=2; i > 0; i--) {
		stride[i] = 1;
		for (j = i-1; j >= 0; j--) {
			if (i > stride_no) {
				stride_dim = dim_size[j];
			}
			else {
				stride_dim = tile_size[j];
			}

			stride[i] *= stride_dim;	
		}
	}
	stride[0] = 1;
	
	for(i=0;i<=t3-1;i++)
		for(j=0;j<=t2-1;j++)
			for(k=0;k<=t1-1;k++)
				memcpy(&pdata[(i*stride[2]+
				               j*stride[1]+k)*e_type_size],
				       &pdata[(i*(d2*d1)+j*d1+k)*e_type_size],
				       e_type_size);
			
}
extern "C" void slave_acemesh_reshape_copyin4d(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int e_type_size, int stride_no) {
	int i,j,k,l;
	char *pdata=(char *)data;
	int dim_size[4];
	int tile_size[4];
	int stride[4];
	int stride_dim;
	
	
	dim_size[0] = d1;
	dim_size[1] = d2;
	dim_size[2] = d3;
	dim_size[3] = d4;
	tile_size[0] = t1;
	tile_size[1] = t2;
	tile_size[2] = t3;
	tile_size[3] = t4;
	
   for (i=3; i > 0; i--) {
		stride[i] = 1;
		for (j = i-1; j >= 0; j--) {
			if (i > stride_no) {
				stride_dim = dim_size[j];
			}
			else {
				stride_dim = tile_size[j];
			}

			stride[i] *= stride_dim;	
		}
	}
   stride[0] = 1;
   
	for(i=d4-1;i>=0;i--)
		for(j=d3-1;j>=0;j--)
			for(k=d2-1;k>=0;k--)
				for(l=d1-1;l>=0;l--)
				memcpy(&pdata[(i*(d3*d2*d1)+j*(d2*d1)+k*d1+l)*e_type_size],
				       &pdata[(i*stride[3]+
				               j*stride[2]+
				               k*stride[1]+l)*e_type_size],
				       e_type_size);
}
extern "C" void slave_acemesh_reshape_copyout4d(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int e_type_size, int stride_no) {
	int i,j,k,l;
	char *pdata=(char *)data;
	int dim_size[4];
	int tile_size[4];
	int stride[4];
	int stride_dim;
	
	
	dim_size[0] = d1;
	dim_size[1] = d2;
	dim_size[2] = d3;
	dim_size[3] = d4;
	tile_size[0] = t1;
	tile_size[1] = t2;
	tile_size[2] = t3;
	tile_size[3] = t4;

	for (i=3; i > 0; i--) {
		stride[i] = 1;
		for (j = i-1; j >= 0; j--) {
			if (i > stride_no) {
				stride_dim = dim_size[j];
			}
			else {
				stride_dim = tile_size[j];
			}

			stride[i] *= stride_dim;	
		}
	}
	stride[0] = 1;
	
	for(i=0;i<=t4-1;i++)
		for(j=0;j<=t3-1;j++)
			for(k=0;k<=t2-1;k++)
				for(l=0;l<=t1-1;l++)
					memcpy(&pdata[(i*stride[3]+
					               j*stride[2]+
					               k*stride[1]+l)*e_type_size],
					       &pdata[(i*(d3*d2*d1)+j*(d2*d1)+k*d1+l)*e_type_size],
					       e_type_size);	
} 
extern "C" void slave_acemesh_reshape_copyin5d(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int d5,int t5,int e_type_size, int stride_no) {
	int h,i,j,k,l;
	char *pdata=(char *)data;
	int dim_size[5];
	int tile_size[5];
	int stride[5];
	int stride_dim;
	
	
	dim_size[0] = d1;
	dim_size[1] = d2;
	dim_size[2] = d3;
	dim_size[3] = d4;
	dim_size[4] = d5;
	tile_size[0] = t1;
	tile_size[1] = t2;
	tile_size[2] = t3;
	tile_size[3] = t4;
	tile_size[4] = t5;

	for (i=4; i > 0; i--) {
		stride[i] = 1;
		for (j = i-1; j >= 0; j--) {
			if (i > stride_no) {
				stride_dim = dim_size[j];
			}
			else {
				stride_dim = tile_size[j];
			}

			stride[i] *= stride_dim;	
		}
	}
	stride[0] = 1;
	
	for(h=d5-1;h>=0;h--)
		for(i=d4-1;i>=0;i--)
			for(j=d3-1;j>=0;j--)
				for(k=d2-1;k>=0;k--)
					for(l=d1-1;l>=0;l--)
					memcpy(&pdata[(h*(d4*d3*d2*d1)+i*(d3*d2*d1)+j*(d2*d1)+k*d1+l)*e_type_size],
					       &pdata[(h*stride[4]+
					               i*stride[3]+
					               j*stride[2]+
					               k*stride[1]+l)*e_type_size],
					       e_type_size);
}
extern "C" void slave_acemesh_reshape_copyout5d(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int d5,int t5,int e_type_size, int stride_no) {
	int h,i,j,k,l;
	char *pdata=(char *)data;
	int dim_size[5];
	int tile_size[5];
	int stride[5];
	int stride_dim;
	
	
	dim_size[0] = d1;
	dim_size[1] = d2;
	dim_size[2] = d3;
	dim_size[3] = d4;
	dim_size[4] = d5;
	tile_size[0] = t1;
	tile_size[1] = t2;
	tile_size[2] = t3;
	tile_size[3] = t4;
	tile_size[4] = t5;

	for (i=4; i > 0; i--) {
		stride[i] = 1;
		for (j = i-1; j >= 0; j--) {
			if (i > stride_no) {
				stride_dim = dim_size[j];
			}
			else {
				stride_dim = tile_size[j];
			}

			stride[i] *= stride_dim;	
		}
	}
	stride[0] = 1;
	
	for(h=0;h<=t5-1;h++)
		for(i=0;i<=t4-1;i++)
			for(j=0;j<=t3-1;j++)
				for(k=0;k<=t2-1;k++)
					for(l=0;l<=t1-1;l++)
						memcpy(&pdata[(h*stride[4]+
						               i*stride[3]+
						               j*stride[2]+
						               k*stride[1]+l)*e_type_size],
						       &pdata[(h*(d4*d3*d2*d1)+i*(d3*d2*d1)+j*(d2*d1)+k*d1+l)*e_type_size],
						       e_type_size);	
} 
extern "C" void slave_acemesh_reshape_copyin6d(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int d5,int t5,int d6,int t6,int e_type_size, int stride_no) {
	int g,h,i,j,k,l;
	char *pdata=(char *)data;
	int dim_size[6];
	int tile_size[6];
	int stride[6];
	int stride_dim;
	
	
	dim_size[0] = d1;
	dim_size[1] = d2;
	dim_size[2] = d3;
	dim_size[3] = d4;
	dim_size[4] = d5;
	dim_size[5] = d6;
	tile_size[0] = t1;
	tile_size[1] = t2;
	tile_size[2] = t3;
	tile_size[3] = t4;
	tile_size[4] = t5;
	tile_size[5] = t6;

	for (i=5; i > 0; i--) {
		stride[i] = 1;
		for (j = i-1; j >= 0; j--) {
			if (i > stride_no) {
				stride_dim = dim_size[j];
			}
			else {
				stride_dim = tile_size[j];
			}

			stride[i] *= stride_dim;	
		}
	}
	stride[0] = 1;
	
	for(g=d6-1;g>=0;g--)
		for(h=d5-1;h>=0;h--)
			for(i=d4-1;i>=0;i--)
				for(j=d3-1;j>=0;j--)
					for(k=d2-1;k>=0;k--)
						for(l=d1-1;l>=0;l--)
						memcpy(&pdata[(g*(d5*d4*d3*d2*d1)+h*(d4*d3*d2*d1)+i*(d3*d2*d1)+j*(d2*d1)+k*d1+l)*e_type_size],
						       &pdata[(g*stride[5]+
						               h*stride[4]+
						               i*stride[3]+
						               j*stride[2]+
						               k*stride[1]+l)*e_type_size],
						       e_type_size);
}
extern "C" void slave_acemesh_reshape_copyout6d(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int d5,int t5,int d6,int t6,int e_type_size, int stride_no) {
	int g,h,i,j,k,l;
	char *pdata=(char *)data;
	int dim_size[6];
	int tile_size[6];
	int stride[6];
	int stride_dim;
	
	dim_size[0] = d1;
	dim_size[1] = d2;
	dim_size[2] = d3;
	dim_size[3] = d4;
	dim_size[4] = d5;
	dim_size[5] = d6;
	tile_size[0] = t1;
	tile_size[1] = t2;
	tile_size[2] = t3;
	tile_size[3] = t4;
	tile_size[4] = t5;
	tile_size[5] = t6;

	for (i=5; i > 0; i--) {
		stride[i] = 1;
		for (j = i-1; j >= 0; j--) {
			if (i > stride_no) {
				stride_dim = dim_size[j];
			}
			else {
				stride_dim = tile_size[j];
			}

			stride[i] *= stride_dim;	
		}
	}
	stride[0] = 1;
	
	for(g=0;g<=t6-1;g++)
		for(h=0;h<=t5-1;h++)
			for(i=0;i<=t4-1;i++)
				for(j=0;j<=t3-1;j++)
					for(k=0;k<=t2-1;k++)
						for(l=0;l<=t1-1;l++)
							memcpy(&pdata[(g*stride[5]+
							                h*stride[4]+
							                i*stride[3]+
							                j*stride[2]+
							                k*stride[1]+l)*e_type_size],
							       &pdata[(g*(d5*d4*d3*d2*d1)+h*(d4*d3*d2*d1)+i*(d3*d2*d1)+j*(d2*d1)+k*d1+l)*e_type_size],
							       e_type_size);	
} 

extern "C" void slave_acemesh_reshape_copyin2d_(void *data,int d1,int t1,int d2,int t2,int e_type_size, int stride_no) {
    slave_acemesh_reshape_copyin2d(data,d1,t1,d2,t2,e_type_size, stride_no);
}

extern "C" void slave_acemesh_reshape_copyout2d_(void *data,int d1,int t1,int d2,int t2,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyout2d(data,d1,t1,d2,t2,e_type_size, stride_no);
}

extern "C" void slave_acemesh_reshape_copyin3d_(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyin3d(data,d1,t1,d2,t2,d3,t3,e_type_size, stride_no);
}
extern "C" void slave_acemesh_reshape_copyout3d_(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyout3d(data,d1,t1,d2,t2,d3,t3,e_type_size, stride_no);
}
extern "C" void slave_acemesh_reshape_copyin4d_(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyin4d(data,d1,t1,d2,t2,d3,t3,d4,t4,e_type_size, stride_no);
}
extern "C" void slave_acemesh_reshape_copyout4d_(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyout4d(data,d1,t1,d2,t2,d3,t3,d4,t4,e_type_size, stride_no);
} 
extern "C" void slave_acemesh_reshape_copyin5d_(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int d5,int t5,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyin5d(data,d1,t1,d2,t2,d3,t3,d4,t4,d5,t5,e_type_size, stride_no);
}
extern "C" void slave_acemesh_reshape_copyout5d_(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int d5,int t5,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyout5d(data,d1,t1,d2,t2,d3,t3,d4,t4,d5,t5,e_type_size, stride_no);
} 
extern "C" void slave_acemesh_reshape_copyin6d_(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int d5,int t5,int d6,int t6,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyin6d(data,d1,t1,d2,t2,d3,t3,d4,t4,d5,t5,d6,t6,e_type_size, stride_no);
}
extern "C" void slave_acemesh_reshape_copyout6d_(void *data,int d1,int t1,int d2,int t2,int d3,int t3,int d4,int t4,int d5,int t5,int d6,int t6,int e_type_size, int stride_no) {
	slave_acemesh_reshape_copyout6d(data,d1,t1,d2,t2,d3,t3,d4,t4,d5,t5,d6,t6,e_type_size, stride_no);
} 

#ifdef NUMA_SUPPORT
extern "C" void acemesh_arraytile(void *arr_addr, char *arr_name, int highest_dim,
                                                    int highest_seg, int rwflag){
    //read-write rwflag: 1(read only), 2(write only), 3(read after write), 4(write after read).

//    hwloc_topology_t topology;
     hwloc_bitmap_t set;
    int err;
//    /* create a topology */
//    err = hwloc_topology_init(&topology);
//    if (err < 0) {
//        std::cout<<"failed to initialize the topology"<<std::endl;
//        assert(0);
//    }
//    err = hwloc_topology_load(topology);
//    if (err < 0) {
//        std::cout<<"failed to load the topology"<<std::endl;
//        hwloc_topology_destroy(topology);
//        assert(0);
//    }
    
    std::map<void *,arrDetails>::iterator it = arrRecords.find(arr_addr);
    //if the current array has not been recorded
    if(it== arrRecords.end()){
        struct arrDetails cur_arrDetails;
        //for the array is read at first appearance, treat it as "touched in serial region"
        if(rwflag==1 || rwflag==3){
            std::cout<<"Warning1: array "<<arr_name<<" has been touched in serial region!"<<std::endl;
            cur_arrDetails.isSerialTouched=true;
        //for the array is written at first appearance, there are 2 cases:
        //1) has been touched in serial region; 2) will be touched in the current dag region;
        }else{
                set = hwloc_bitmap_alloc();
                char *s;
                //check the second page to avoid passive touch phenomenon
                err= hwloc_get_area_memlocation(topology, arr_addr+4096, 1, set,
                    HWLOC_MEMBIND_STRICT|HWLOC_MEMBIND_BYNODESET);
                hwloc_bitmap_asprintf(&s, set);
                // std::cout<<"!array"<<arr_name<<"["<<i<<"]"<<s<<std::endl;
            // }
            //1) has been touched in serial region
                int tmp=atoi(&s[2]);
                //if(s[9]-'1'==0||s[9]-'1'==1){
                if(tmp>0){
                  std::cout<<s<<"...Warning2: array "<<arr_name <<" has been touched in serial region!"<<std::endl;
                  cur_arrDetails.isSerialTouched=true;
                }
            // std::cout<<s[9]<<"   "<<s[9]-0<<std::endl;
            //b) will be touched in the current dag region
            //else{}
        }
        //record infos of the current array
        cur_arrDetails.highest_dim=highest_dim;
        cur_arrDetails.highest_seg=highest_seg;
        arrRecords.insert(std::make_pair(arr_addr,cur_arrDetails));    
    //if the current array has been recorded, check for conflicts
    }else{
        //only check the array which touched in the non-serial area可
        // std::cout<<"Warning3: array "<<arr_name <<" has been recorded!"<<std::endl;
        if(it->second.isSerialTouched==false){
            if(it->second.highest_dim!=highest_dim ||
                it->second.highest_seg!=highest_seg){
                std::cout<<"Warning: array "<<arr_name<<" is inconsistent with the recorded"<<std::endl;
            }
        }
    }

}
extern "C" void acemesh_arraytile_(void *arr_addr, char *arr_name, int highest_dim,int highest_seg, int rwflag){
    acemesh_arraytile(arr_addr,arr_name,highest_dim,highest_seg,rwflag);
}

extern "C" void acemesh_affinity_from_arraytile(int highest_seg_id,int highest_seg_num,int num_threads){
    int affi;
    int nodeid;
    int tempid;
    int nodenum;
    nodenum=ceil(num_threads/CORENUM);
    
//    if(num_threads<=CORENUM){
//        nodeid=0;
//    }else{
/*        if(highest_seg_id<highest_seg_num/2){
            nodeid=0;
        }else{
            nodeid=1;
        }
*/
           tempid=ceil((double)highest_seg_num/nodenum);
           nodeid=(highest_seg_id)/tempid;
//printf("%d,%d,%d,%d,",highest_seg_num,nodenum,tempid,nodeid);
//    }
//    if(num_threads<=CORENUM)
//      affi=task_count[nodeid]%num_threads;
//    else
      tempid=num_threads%CORENUM;
      if (tempid==0)
        affi=task_count[nodeid]%CORENUM+CORENUM*nodeid;
      else
        affi=task_count[nodeid]%(num_threads%CORENUM)+CORENUM*nodeid;
    
    //printf("%d,%d.    ",task_count[nodeid],affi);
    acemesh_task_set_affinity(affi);
    ++task_count[nodeid];
}

extern "C" void acemesh_affinity_from_arraytile_(int highest_seg_id,int highest_seg_num,int num_threads){
    acemesh_affinity_from_arraytile(highest_seg_id,highest_seg_num,num_threads);
}


#endif

