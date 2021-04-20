#ifndef _ACE_MESH_RUNTIMEC_
#define _ACE_MESH_RUNTIMEC_
#include <stdbool.h>
typedef int Error_Code;
#define ACEMESH_OK  0

//NEIGHBOR relationship
#define NEIGHBOR_NONE      0x00000000
#define NEIGHBOR_UP        0x00000001
#define NEIGHBOR_DOWN      0x00000002
#define NEIGHBOR_LEFT      0x00000004
#define NEIGHBOR_RIGHT     0x00000008
#define NEIGHBOR_FRONT     0x00000010
#define NEIGHBOR_BACK      0x00000020
#define NEIGHBOR_ALL       0x0000ffff

//the read/write flag for addr
#define INOUT_NONE 0
#define IN         1
#define OUT        2
#define INOUT      3

//the type flag for the area that stands for by addr
#define NORMAL               1 
#define SHADE                2
#define UNSHADE              3
#define SHADE_AND_UNSHADE    4

//for task type
//typedef int task_type; 
#define NOT_SET 0
#define STENCIL_TASK    1
#define NOAFFINITY_TASK 2
#define BLOCKING_TASK 3   //for mpi_wait, or mpi_send,mpi_recv etc blocking ops.
extern int num_registers;
//control dependence of MPI programs
#define SERIALIZE_POSTWAIT      0x1111UL

//for c interface, struct arr
struct ptr_array
{
	int len;
    void** arr;
};
//end

	Error_Code begin_split_task(char * loop_info);
	Error_Code begin_split_task();
	Error_Code end_split_task();

	void spawn_and_wait();
	void spawn_and_wait(int print_graph);
//#ifdef CONCURRENT_CONSTRUCT_GRAPH
    void wait_for_all_task();
//#endif

//clang interfaces
/***********************************************************************/
/*Discussion on struct args:                                           */
/*actually, we should use interface as low as possible,                */
/*not using struct args, but then task_generator_with_neighbors() will */
/*need more argument, int* num_neighbors besides the packed args, args */
/*it is strange to pack them together, So let it intact                */

typedef void (*TASK_FUNCPTR)(void *args);

#ifndef SWF
  typedef void (*NEIGHBOR_FUNCPTR)(struct ptr_array *neighbor_addrs,  void *args);
#else
  typedef void (*NEIGHBOR_FUNCPTR)(void *args);
#endif
#ifdef  __cplusplus
extern "C"
{
#endif
void aceMesh_MPI_rank(int rank);
void acemesh_runtime_init(int total_threads);
void acemesh_runtime_shutdown();
void acemesh_runtime_shutdown_with_proc_id(int proc_id);
//Acemesh task generator
void acemesh_task_generator(TASK_FUNCPTR taskfptr,void* args,  unsigned int args_size);
void acemesh_task_generator_with_neighbors(TASK_FUNCPTR taskfptr, void* args,  unsigned int args_size, 
             void* cxx_this_pointer, NEIGHBOR_FUNCPTR get_neighbors_funcptr, void *neighbor_args);

//Handler for #pragma acemesh data clause
void acemesh_push_wrlist(int argc, void *addr, int access_flag, ...);
void acemesh_push_rlist(int argc, void *addr, int access_flag, ...);
void acemesh_push_wlist(int argc, void *addr, int access_flag, ...);

int acemesh_dag_start_vec(int dagNo, int *int_vec, int n1, double *float_vec, int n2);
int acemesh_dag_start(int dagNo);
void acemesh_begin_split_task(char * loop_info);
void acemesh_end_split_task();
void acemesh_spawn_and_wait(int print_graph);	
void acemesh_specify_end_tasks();
//#ifdef CONCURRENT_CONSTRUCT_GRAPH
    void acemesh_wait_for_all_task();
//#endif

void *acemesh_malloc_obj(unsigned int args_size);

//control task attributes (for the ci_task just now generated)
void acemesh_task_set_type(int type);
void acemesh_task_set_affinity(int id);
void acemesh_reset_affinity();


void acemesh_set_suspend(int mode);
void acemesh_mpi_rank(int rank);
//add new,tsl
 int acemesh_get_thread_id();
// int acemesh_get_thread_num();

//numa related
void acemesh_arraytile(void *cur_arr,char *arr_name,int highest_dim,int highest_seg,int rwflag);
void acemesh_affinity_from_arraytile(int highest_seg_id,int highest_seg_num,int num_threads);

#ifdef  __cplusplus
}
#endif

#endif
