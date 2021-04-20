!************************
!  fortran interface
!************************
!#ifndef OLD_STYLE
#define ACEMESH_OK  0
!!#include "aceMesh_task.h"
!!#define  ACEMESH_OK  0
!#else
!    integer,parameter :: ACEMESH_OK = 0!
 
#define NEIGHBOR_NONE  z'00000000'
#define NEIGHBOR_UP  z'00000001'
#define NEIGHBOR_DOWN  z'00000002'
#define NEIGHBOR_LEFT  z'00000004'
#define NEIGHBOR_RIGHT  z'00000008'
#define NEIGHBOR_FRONT  z'00000010'
#define NEIGHBOR_BACK  z'00000020'
#define NEIGHBOR_ALL  z'0000ffff'
!*******************
! addr flag
!*******************
#define ADDR_INOUT_NONE  0
#define ADDR_IN 1
#define ADDR_OUT  2
#define ADDR_INOUT  3

!*******************
!area flag
!*******************
#define NORMAL  1
#define SHADE  2
#define UNSHADE  3
#define HADE_AND_UNSHADE  4

!*******************
!task type
!*******************
#define NOT_SET  0
#define STENCIL_TASK  1
#define NOAFFINITY_TASK  2
#define BLOCKING_TASK  3
#define MAX_NEIGHBORS  32
!******************
!control dependence of MPI programs
!*****************
#define SERIALIZE_POSTWAIT  z'0001111'

#define C_NULL_PTR  0

#ifdef SWFUNC

#define slave_acemesh_athread_get athread_get 
#define slave_acemesh_athread_put athread_put 

#define slave_acemesh_athread_get_ athread_get_
#define slave_acemesh_athread_put_ athread_put_ 

#endif

#define SIZEOF_CHARACTER   1
#define SIZEOF_LOGICAL1    1
#define SIZEOF_LOGICAL2    2
#define SIZEOF_LOGICAL4    4
#define SIZEOF_LOGICAL8    8
#define SIZEOF_INTEGER1   1 
#define SIZEOF_INTEGER2  2
#define SIZEOF_INTEGER4   4
#define SIZEOF_INTEGER8   8
#define SIZEOF_REAL4   4
#define SIZEOF_REAL8   8
#define SIZEOF_DOUBLEPRECISION8   8
#define SIZEOF_CRAYPOINTER8   8

!define acemesh_get_loop_tile_start_index_by_tile_no(loop_tile_size, loop_tile_no, loop_index_lb) max(loop_tile_no* loop_tile_size + loop_index_lb - mod(loop_index_lb,loop_tile_size), loop_index_lb)
!define acemesh_get_loop_tile_end_index_by_tile_no(loop_tile_size, loop_tile_no, loop_index_lb, loop_index_ub) min ((loop_tile_no+1)* loop_tile_size + loop_index_lb - mod(loop_index_lb,loop_tile_size)-1, loop_index_ub)
!define acemesh_get_data_tile_start_index_by_index(data_tile_size, compute_index, array_dim_lb) max(compute_index - (compute_index % data_tile_size), array_dim_lb)
!*****************************************************************
! clang functions 
!*****************************************************************      

INTERFACE
!********************************************
   subroutine acemesh_runtime_init(total_threads)             
      integer*4,value :: total_threads
   end subroutine acemesh_runtime_init
   
   subroutine acemesh_runtime_shutdown()  
   end subroutine acemesh_runtime_shutdown

   subroutine acemesh_runtime_shutdown_with_proc_id(proc_id)       
     integer*4,value :: proc_id
   end subroutine acemesh_runtime_shutdown_with_proc_id
 
!***************************************
! task generators
!***************************************   
  subroutine acemesh_task_generator(taskfptr,args,args_size) bind(C) 
     use ISO_C_BINDING
     type(c_funptr),value ::taskfptr
     type(c_ptr),value :: args
     integer(c_int),value :: args_size
  end subroutine acemesh_task_generator
   subroutine acemesh_task_generator_with_neighbors(taskfptr,   &
        args,args_size,cxx_this_pointer,get_neighbors_funcptr,  &
        neighbor_args) bind(C)
      use ISO_C_BINDING
      type(c_funptr),value :: taskfptr
      type(c_ptr),value :: args
      integer(c_int),value :: args_size
      type(c_ptr),value :: cxx_this_pointer
      type(c_funptr),value :: get_neighbors_funcptr
      type(c_ptr),value :: neighbor_args
   end subroutine acemesh_task_generator_with_neighbors
   
   subroutine acemesh_task_generator_swf(taskfuncno,args,args_size)
     integer*4,value :: taskfuncno
     integer*8,value :: args
     integer*4,value :: args_size
   end subroutine acemesh_task_generator_swf
   subroutine acemesh_task_generator_swf2(taskfuncptr,args,args_size) 
     interface
       subroutine taskfuncptr() BIND(C)
       end subroutine taskfuncptr  
     end interface
     integer*8,value :: args
     integer*4,value :: args_size
   end subroutine acemesh_task_generator_swf2

   
   subroutine acemesh_task_generator_with_neighbors_swf(taskfuncno,   &
        args,args_size,cxx_this_pointer,get_neighbors_funcno,  &
        neighbor_args)
     integer*4,value :: taskfuncno
     integer*8,value :: args
     integer*4,value :: args_size
     integer*8,value :: cxx_this_pointer
     integer*4,value :: get_neighbors_funcno
     integer*8,value :: neighbor_args
   end subroutine acemesh_task_generator_with_neighbors_swf
   subroutine acemesh_task_generator_with_neighbors_swf2(taskfuncptr,   &
        args,args_size,cxx_this_pointer,get_neighbors_funcptr,  &
        neighbor_args)
      interface
        subroutine taskfuncptr() BIND(C)
        end subroutine taskfuncptr
      end interface
      integer*8,value :: args
      integer*4,value :: args_size
      integer*8,value :: cxx_this_pointer
      interface
        subroutine get_neighbors_funcptr() BIND(C)
        end subroutine get_neighbors_funcptr
      end interface
      integer*8,value :: neighbor_args
   end subroutine acemesh_task_generator_with_neighbors_swf2
!*****************************************
! data descriptions
!*****************************************
#ifdef OLD_STYLE
   subroutine acemesh_push_wrlist1(argc,addr,access_flag) bind(C)
      use ISO_C_BINDING
      integer(c_int),value :: argc
      type(c_ptr),value :: addr
      integer(c_int),value :: access_flag
   end subroutine acemesh_push_wrlist1
   subroutine acemesh_push_rlist1(argc,addr,access_flag) bind(C)
      use ISO_C_BINDING
      integer(c_int),value :: argc
      type(c_ptr),value :: addr
      integer(c_int),value :: access_flag
   end subroutine acemesh_push_rlist1
   subroutine acemesh_push_wlist1(argc,addr,access_flag) bind(C)     
      use ISO_C_BINDING
      integer(c_int),value :: argc
      type(c_ptr),value :: addr
	  integer(c_int),value :: access_flag
   end subroutine acemesh_push_wlist1
#else
   subroutine acemesh_push_wrlist1(argc,addr,access_flag)      
     integer*4,value :: argc
     integer*8,value :: addr
     integer*4,value :: access_flag
   end subroutine acemesh_push_wrlist1
   subroutine acemesh_push_rlist1(argc,addr,access_flag)      
     integer*4,value :: argc
     integer*8,value :: addr
     integer*4,value :: access_flag
   end subroutine acemesh_push_rlist1
   subroutine acemesh_push_wlist1(argc,addr,access_flag)           
    integer*4,value :: argc
    integer*8,value :: addr
	  integer*4,value :: access_flag
   end subroutine acemesh_push_wlist1
#endif
   subroutine acemesh_push_neighbor_addr(addr)
    integer*8,value :: addr
   end subroutine acemesh_push_neighbor_addr

   function acemesh_dag_start_vec(dagNo,int_vec,n1,float_vec,n2)      
      integer*4 :: acemesh_dag_start_vec
      integer*4,value :: dagNo
      integer*8,value :: int_vec
      integer*4,value :: n1
      integer*8,value :: float_vec
      integer*4,value :: n2
   end function acemesh_dag_start_vec

   function acemesh_dag_start(dagNo)      
      integer*4 :: acemesh_dag_start
      integer*4,value :: dagNo
   end function acemesh_dag_start
   
   subroutine acemesh_begin_split_task(loop_info)         
      character*(*) :: loop_info
     !integer*8,value:: loop_info
   end subroutine acemesh_begin_split_task
   
   subroutine acemesh_end_split_task()   
   end subroutine acemesh_end_split_task
  
   subroutine acemesh_spawn_and_wait(print_graph)       
      integer*4,value :: print_graph
   end subroutine acemesh_spawn_and_wait

!*******************************
! control task attributes
!*******************************
   subroutine acemesh_task_set_type(type)             
      integer*4,value :: type
   end subroutine acemesh_task_set_type
   
   subroutine acemesh_task_set_affinity(id)              
      integer*4,value :: id
   end subroutine acemesh_task_set_affinity

   subroutine acemesh_reset_affinity
   end subroutine acemesh_reset_affinity

   subroutine acemesh_task_set_priority_id(id)              
      integer*4,value :: id
   end subroutine acemesh_task_set_priority_id

   function acemesh_task_get_priority_id()
      integer*4 :: acemesh_task_get_priority_id
   end function acemesh_task_get_priority_id

   subroutine acemesh_set_suspend(mode)  	
	  integer*4,value :: mode
   end subroutine acemesh_set_suspend

!   subroutine acemesh_set_event(h1,h2)	 
!      integer*8,value :: h1
!      integer*8,value :: h2
!   end subroutine acemesh_set_event
   subroutine acemesh_set_mtest_handle(h1, h2,kind1, kind2)
      integer*8,value :: h1
      integer*8,value :: h2
      integer*4,value :: kind1
      integer*4,value :: kind2
   end subroutine acemesh_set_mtest_handle
   subroutine acemesh_task_map_master()
   end subroutine acemesh_task_map_master
   
   subroutine acemesh_mpi_rank(rank)	
	   integer*4,value :: rank
   end subroutine acemesh_mpi_rank

   function acemesh_get_thread_id()
      integer*4 :: acemesh_get_thread_id
   end function acemesh_get_thread_id

   function acemesh_get_loop_tile_num(loop_tile_size, loop_index_lb, loop_index_ub)
		integer*4 :: acemesh_get_loop_tile_num
		integer*4, value :: loop_tile_size
		integer*4, value :: loop_index_lb
		integer*4, value :: loop_index_ub
   end function acemesh_get_loop_tile_num

   function acemesh_get_tile_no_by_index(tile_size, compute_index, loop_index_lb)
		integer*4 :: acemesh_get_tile_no_by_index
		integer*4, value :: tile_size
		integer*4, value :: compute_index
		integer*4, value :: loop_index_lb
   end function acemesh_get_tile_no_by_index

	function acemesh_get_loop_tile_start_index_by_tile_no(loop_tile_size, loop_tile_no, loop_index_lb)
		integer*4 :: acemesh_get_loop_tile_start_index_by_tile_no
    integer*4, value :: loop_tile_size
    integer*4, value :: loop_tile_no
    integer*4, value :: loop_index_lb
  end function acemesh_get_loop_tile_start_index_by_tile_no

  function acemesh_get_loop_tile_end_index_by_tile_no(loop_tile_size, loop_tile_no, loop_index_lb, loop_index_ub)
		integer*4 :: acemesh_get_loop_tile_end_index_by_tile_no
		integer*4, value :: loop_tile_size
		integer*4, value :: loop_tile_no
		integer*4, value :: loop_index_lb		
		integer*4, value :: loop_index_ub
  end function acemesh_get_loop_tile_end_index_by_tile_no

  function acemesh_get_data_tile_start_index_by_index(data_tile_size, compute_index, array_dim_lb)
		integer*4 :: acemesh_get_data_tile_start_index_by_index
		integer*4, value :: data_tile_size
		integer*4, value :: compute_index
		integer*4, value :: array_dim_lb
  end function acemesh_get_data_tile_start_index_by_index
 
   function acemesh_get_loop_tile_num_with_init_offset(loop_tile_size, loop_index_lb, loop_index_ub, init_offset)
		integer*4 :: acemesh_get_loop_tile_num_with_init_offset
		integer*4, value :: loop_tile_size
		integer*4, value :: loop_index_lb
		integer*4, value :: loop_index_ub
		integer*4, value :: init_offset
   end function acemesh_get_loop_tile_num_with_init_offset


   function acemesh_get_loop_tile_start_index_by_tile_no_with_init_offset(loop_tile_size, loop_tile_no, loop_index_lb, init_offset)
		integer*4 :: acemesh_get_loop_tile_start_index_by_tile_no_with_init_offset
        integer*4, value :: loop_tile_size
        integer*4, value :: loop_tile_no
        integer*4, value :: loop_index_lb
        integer*4, value :: init_offset
   end function acemesh_get_loop_tile_start_index_by_tile_no_with_init_offset

   function acemesh_get_loop_tile_end_index_by_tile_no_with_init_offset(loop_tile_size, loop_tile_no, loop_index_lb, loop_index_ub, init_offset)
		integer*4 :: acemesh_get_loop_tile_end_index_by_tile_no_with_init_offset
		integer*4, value :: loop_tile_size
		integer*4, value :: loop_tile_no
		integer*4, value :: loop_index_lb		
		integer*4, value :: loop_index_ub
		integer*4, value :: init_offset
   end function acemesh_get_loop_tile_end_index_by_tile_no_with_init_offset

   function acemesh_get_data_tile_start_index_by_index_with_init_offset(data_tile_size, compute_index, array_dim_lb, init_offset)
		integer*4 :: acemesh_get_data_tile_start_index_by_index_with_init_offset
		integer*4, value :: data_tile_size
		integer*4, value :: compute_index
		integer*4, value :: array_dim_lb
		integer*4, value :: init_offset
   end function acemesh_get_data_tile_start_index_by_index_with_init_offset

   function acemesh_get_tile_no_by_index_with_init_offset(tile_size, index, lb, init_offset)
        integer*4 :: acemesh_get_tile_no_by_index_with_init_offset
        integer*4, value :: tile_size
        integer*4, value :: index
        integer*4, value :: lb
        integer*4, value :: init_offset
   end function acemesh_get_tile_no_by_index_with_init_offset 
   function slave_acemesh_athread_get(mode, src, dest, len, reply, mask, stride, bsize)
		integer*4 :: slave_acemesh_athread_get
		integer*4, value :: mode
		integer*8, value :: src
		integer*8, value :: dest
		integer*4, value :: len
		integer*8, value :: reply
		integer*4, value :: mask
		integer*4, value :: stride
		integer*4, value :: bsize
  end function slave_acemesh_athread_get

  function slave_acemesh_athread_put(mode, src, dest, len, reply, stride, bsize)
		integer*4 :: slave_acemesh_athread_put
		integer*4, value :: mode
		integer*8, value :: src
		integer*8, value :: dest
		integer*4, value :: len
		integer*8, value :: reply
		integer*4, value :: stride
		integer*4, value :: bsize
  end function  slave_acemesh_athread_put

	subroutine slave_acemesh_wait_reply(reply, count)
		integer*8, value :: reply
		integer*4, value :: count
  end subroutine slave_acemesh_wait_reply

  subroutine acemesh_set_thread_num(thread_num)
     integer*4, value :: thread_num
  end subroutine acemesh_set_thread_num

  subroutine acemesh_specify_end_tasks()
  end subroutine acemesh_specify_end_tasks

	subroutine slave_acemesh_reshape_copyin2d(A,d1,t1,d2,t2,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine slave_acemesh_reshape_copyin2d

	subroutine slave_acemesh_reshape_copyin3d(A,d1,t1,d2,t2,d3,t3,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::d3
		integer*4,value::t3
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine slave_acemesh_reshape_copyin3d

	subroutine slave_acemesh_reshape_copyin4d(A,d1,t1,d2,t2,d3,t3,d4,t4,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::d3
		integer*4,value::t3
		integer*4,value::d4
		integer*4,value::t4
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine slave_acemesh_reshape_copyin4d

	subroutine slave_acemesh_reshape_copyin5d(A,d1,t1,d2,t2,d3,t3,d4,t4,d5,t5,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::d3
		integer*4,value::t3
		integer*4,value::d4
		integer*4,value::t4
		integer*4,value::d5
		integer*4,value::t5
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine slave_acemesh_reshape_copyin5d

	subroutine slave_acemesh_reshape_copyin6d(A,d1,t1,d2,t2,d3,t3,d4,t4,d5,t5,d6,t6,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::d3
		integer*4,value::t3
		integer*4,value::d4
		integer*4,value::t4
		integer*4,value::d5
		integer*4,value::t5
		integer*4,value::d6
		integer*4,value::t6		
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine slave_acemesh_reshape_copyin6d


	
	subroutine slave_acemesh_reshape_copyout2d(A,d1,t1,d2,t2,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine slave_acemesh_reshape_copyout2d

	subroutine slave_acemesh_reshape_copyout3d(A,d1,t1,d2,t2,d3,t3,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::d3				
		integer*4,value::t3
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine slave_acemesh_reshape_copyout3d

	subroutine slave_acemesh_reshape_copyout4d(A,d1,t1,d2,t2,d3,t3,d4,t4,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::d3
		integer*4,value::t3
		integer*4,value::d4 
		integer*4,value::t4
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine
	subroutine slave_acemesh_reshape_copyout5d(A,d1,t1,d2,t2,d3,t3,d4,t4,d5,t5,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::d3
		integer*4,value::t3
		integer*4,value::d4 
		integer*4,value::t4
		integer*4,value::d5 
		integer*4,value::t5		
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine

	subroutine slave_acemesh_reshape_copyout6d(A,d1,t1,d2,t2,d3,t3,d4,t4,d5,t5,d6,t6,e_type_size,stride_no)
		integer*8,value::A
		integer*4,value::d1
		integer*4,value::t1
		integer*4,value::d2
		integer*4,value::t2
		integer*4,value::d3
		integer*4,value::t3
		integer*4,value::d4 
		integer*4,value::t4
		integer*4,value::d5 
		integer*4,value::t5
		integer*4,value::d6 
		integer*4,value::t6		
		integer*4,value::e_type_size
		integer*4,value::stride_no
	end subroutine	

!*******************************
! NUMA related
!*******************************
  subroutine acemesh_arraytile(arr_addr,arr_name,highest_dim,highest_seg,rwflag)
	integer*8, value ::arr_addr
	character*(*) :: arr_name
	integer*4, value ::highest_dim
	integer*4, value ::highest_seg
	integer*4, value ::rwflag
  end subroutine acemesh_arraytile
  
  subroutine acemesh_affinity_from_arraytile(li_i3,lt_num_0,thread_num)
	integer*4, value ::li_i3
	integer*4, value ::lt_num_0
	integer*4, value ::thread_num
  end subroutine acemesh_affinity_from_arraytile

end interface

    
