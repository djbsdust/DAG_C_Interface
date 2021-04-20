!****************************************************
!  task partition only on two dimensions <Z,Y-split,X-split>
!                       i.e. in loop split<k,j-split,i-split> 
!  usage: make see fdag in makefile, run as ./3d7p 
!***************************************************
#define alpha 0.0876
#define beta 0.0765

     module stencil_dag_related
     use  ISO_C_BINDING
     use dag_struct
     real(c_double),allocatable ::p(:,:,:,:)
     real(c_double),allocatable ::t(:,:,:,:)
     type ARG_TYPE    
       integer(c_int) starti,endi,startj,endj
       integer(c_int) sizeK
       integer(c_int) dest,src
     end type
     end module stencil_dag_related

     program main
     use  ISO_C_BINDING
     use  stencil_dag_related
     implicit none
     ! fortran array, default lower bound is 1
#include "aceMesh_runtime_f.h"
    INTERFACE
     subroutine neighbor(neighbor_addrs,args) 
       use  stencil_dag_related
      implicit none
      type(ARG_TYPE),target :: args
      type(ptr_array),intent(inout)::neighbor_addrs
     END
     subroutine core_3d7p(args)
       use  stencil_dag_related
      implicit none
      type(ARG_TYPE),target :: args
     END
    integer function check(iter)
     use  stencil_dag_related
     implicit none
        integer(c_int) iter
    end
    END INTERFACE
     external timer_read
     real (c_double) timer_read,tt
     integer(c_int) num_thread,ret
     character(3)  thread
     integer(c_int) itt,i,j,endj,endi,num 
     integer(c_int) arg_size,dest,src
     type(ARG_TYPE) :: each
     integer(c_int),pointer :: pp =>null()
     allocate (p(SIZEZ,SIZEY,SIZEX,2))
    allocate (t(SIZEZ,SIZEY,SIZEX,2))
     !num_thread = 1
     call my_init()
     !****************************************************************
     ! we suggest use ACEMESH_NUM_THREADS as specified by dag library
     ! we do not encourage to specify num_threads in the source program
     ! we init using argument=0
     !****************************************************************
     !      delete the following lines!
     !call get_environment_variable("NUM_THREADS",thread)
     !read (thread,'(I3)') num_thread
     !***************************************************************
     call timer_clear(1)
     call acemesh_runtime_init(0)
     
     call timer_start(1)
     !num=1
     do itt=1,ITER
        !the first iteration, dest=2
        !dest,src in[1,2]
        dest=mod(itt, 2)+1
        src=mod(itt+1,2)+1
        call acemesh_begin_split_task('3d7p')
        do j=2,SIZEY-1,BLKY
          if(j+BLKY-1 > SIZEY-1)then
            endj=SIZEY-1
          else
            endj=j+BLKY-1
          endif
          do i=2,SIZEX-1,BLKX
            if(i+BLKX-1>SIZEX-1)then
              endi=SIZEX-1
            else
              endi=i+BLKX-1
            endif
            each%starti=i
            each%endi=endi
            each%startj=j
            each%endj=endj
            each%sizeK=SIZEZ
            each%dest=dest
            each%src=src
            arg_size = sizeof(each)
            call acemesh_push_wlist1(1,c_loc(p(1,i,j,dest)),NORMAL)
            call acemesh_push_rlist1(1,c_loc(p(1,i,j,src)),NORMAL)
            call acemesh_task_generator_with_neighbors(c_funloc(core_3d7p),    &
                  c_loc(each),arg_size,c_loc(pp),c_funloc(neighbor),c_loc(each))
            call acemesh_task_set_type(STENCIL_TASK)
          enddo
        enddo
        call acemesh_end_split_task()
     enddo ! time iteration
     call acemesh_spawn_and_wait(99)
     call timer_stop(1)
     
     call acemesh_runtime_shutdown()
     tt=timer_read(1)
     ret = check(ITER)
     if(ret == -1)then
        print *,'error'
     else
        print *,'dag time including graph build:',tt
        print *,'check ok!'
     endif
    deallocate(p)
    deallocate(t)
!    deallocate(c_array)
     END
     !contain
      !*****************************************
      !  neighbor addrs
      !*****************************************
      subroutine neighbor(neighbor_addrs,args)
    use  stencil_dag_related
    use ISO_C_BINDING 
       implicit none
#include "aceMesh_runtime_f.h"
      integer(c_int) starti,startj,sz,i
      type(ARG_TYPE),target :: args
      type(ptr_array) :: neighbor_addrs
      integer(c_int),target :: num_neigh
      integer(c_int) dest,src,num
      type(c_ptr),pointer::array(:)
      sz=MAX_NEIGHBORS
      call c_f_pointer(neighbor_addrs%arr,array,[sz])
      starti = args%starti
      startj = args%startj
      src=args%src
      num=0;
      if (starti>=BLKX+2)then
         num=num+1
         array(num) = c_loc(p(1,starti-BLKX,startj,src))
         !write(*,"(1x, Z12)") loc(u(1,startj,starti-BLKX,src))
      endif
      if (starti<SIZEX-BLKX)then
         num=num+1
         array(num) = c_loc(p(1,starti+BLKX,startj,src))
         !write(*,"(1x, Z12)") loc(u(1,startj,starti+BLKX,src))
      endif
      if (startj>=BLKY+2)then
         num=num+1
         array(num) = c_loc(p(1,starti,startj-BLKY,src)) 
         !write(*,"(1x, Z12)") loc(u(1,startj-BLKY,starti,src))
      endif
      if (startj<SIZEY-BLKY)then
         num=num+1
         array(num) = c_loc(p(1,starti,startj+BLKY,src)) 
         !write(*,"(1x, Z12)") loc(u(1,startj+BLKY,starti,src))
      endif
      neighbor_addrs%len=num
      return
     end subroutine neighbor

     !**********************************
     !  task's computation code
     !**********************************
      subroutine core_3d7p(args)
    use  stencil_dag_related
    use ISO_C_BINDING
      implicit none
      type(ARG_TYPE),target :: args
     integer(c_int) starti,endi,startj,endj
     integer(c_int) i,j,k,sizek,dest,src
     starti = args%starti
     endi = args%endi
     startj = args%startj
     endj = args%endj
     dest = args%dest
     src = args%src
     sizek = args%sizeK
     !NOTE: you can not print to stdout here,  
     !forrtl: severe (40): recursive I/O operation, unit -1, file unknown
     do i=starti,endi
     do j=startj,endj
     do k=2, sizek - 1
        p(k,j,i,dest) = alpha * p(k,j,i,src) +beta * (p(k,j,i+1,src) + &
                p(k,j+1,i,src)+ p(k+1,j,i,src)+ p(k,j,i-1,src) &
                + p(k,j-1,i,src)+ p(k-1,j,i,src))
     enddo
     enddo
     enddo
     return
     end subroutine core_3d7p
    
!**********************************************
!     data initialization function 
!**********************************************
     subroutine my_init()
    use  stencil_dag_related
     implicit none
     integer(c_int) i,j,k
     do k=1,SIZEX
     do j=1,SIZEY
     do i=1,SIZEZ
         p(i,j,k,1)= (k*2.5 + j*3.3 + i*0.5)/3.3
         t(i,j,k,1)=p(i,j,k,1)
     enddo
     enddo
     enddo
     !array2's boundary data
     do k=1,SIZEX,SIZEX-1
     do j=1,SIZEY,SIZEY-1
     do i=1,SIZEZ,SIZEZ-1
         p(i,j,k,2)= (k*2.5 + j*3.3 + i*0.5)/3.3
         t(i,j,k,2)=p(i,j,k,2)
     enddo
     enddo
     enddo

     return
     end subroutine my_init

!**********************************************
!     check function 
!**********************************************
     integer function check(iter)
     use  stencil_dag_related
     implicit none
     integer(c_int) iter,i,j,k,itt,dest,src
     integer(c_int) fab
         
     dest=1  !this is for special case of zero iteration stencil
     do  itt = 1, iter 
       dest=mod(itt,2)+1
       src =mod(itt+1,2)+1

       do k=2, SIZEX-1
       do j=2, SIZEY-1
       do i=2, SIZEZ-1
          t(i,j,k,dest) = alpha * t(i,j,k,src) +beta * (t(i,j,k+1,src) +  &
                t(i,j+1,k,src)+ t(i+1,j,k,src)+ t(i,j,k-1,src)+ t(i,j-1,k,src)+t(i-1,j,k,src))
       enddo
       enddo
       enddo
     enddo    
     !check...
     do k=2,SIZEX-1
     do j=2,SIZEY-1
     do i=2,SIZEZ-1
        fab=abs(t(i,j,k,dest)-p(i,j,k,dest))
        if(fab >= 1e-16)then
            print *,'i=',i,'j=',j,'k=',k
            print *,t(i,j,k,dest),'vs',p(i,j,k,dest)
            print *,'different!'
            check = -1
            return
        endif
     enddo
     enddo
     enddo
     check = 0
     return
     end function check

     !end program main

