! CALC1, and CALC2 are partially parallelized,
! CALC3 and CALC3Z are not parallelized yet,
! there are two separate dag graphs currently
! don't think it will give much performance gains

! about DAG parallelization
! DO NOT KNOW how to assign representative address for a certain data region in a complex program as swim
! because so many overlaped regions from different loop nests
! need an algorithm, But it currently not exist

! calc1-loop writes CU(i+1,j),CV(i,j+1),Z(i+1,j+1), 
!            reads P(0,1),(1,0),(0,0),(1,1)
!            reads V(0,1),(1,1),(0,0)
!            reads U(1,0),(1,1),(0,0)
! calc2-loop write: unew(i+1,j),vnew(i,j+1),pnew(i,j);
!            read:Z/CV/CU,(0,0),(1,1),(1,0),(0,1)
!            read H:(1,0),(0,1),(0,0)
!            read vold(0,1)
! calc3-loop write: uold(i,j),vold(i,j),pold(i,j),u(i,j),v(i,j),p(i,j), 
!            read,also no offset

! WHAT IF one array is written in two differnent loops with different offsets?
! for example,
!         in one loop,   CU(i+1,j)=...
!         second loop      ... =... CU(i,J)...
!         in third loop, CU(i,J+1)=...
!         fourth loop,     ...=CU(i+1,j)...CU(i+1,j+1)...
! how to choose address for each loops above?


! it chances that the generated dependence graph equals to the correct program dependences among loop tiles
! but we dont have an algorithm to guarantee that!

! about variables, ip1 means 'i plus 1'
!                  im1 means 'i minus 1'

        PROGRAM main 
        use swim_dag_related
        IMPLICIT NONE
#include "aceMesh_runtime_f.h"
        INTEGER(c_int) I,J,NCYCLE
        REAL(c_double) TIME_START,TIME_END
        character(3) threads 
        allocate(U(NN,NM))
        allocate(V(NN,NN))
        allocate(p(NN,NM))
        allocate(UNEW(NN,NM))
        allocate(VNEW(NN,NM))
        allocate(PNEW(NN,NM))
        allocate(UOLD(NN,NM))
        allocate(VOLD(NN,NM))
        allocate(POLD(NN,NM))
        allocate(CU(NN,NM))
        allocate(CV(NN,NM))
        allocate(Z(NN,NM))
        allocate(PSI(NN,NM))
        allocate(H(NN,NM))
        WRITE(6,*) ' SPEC benchmark 171.swim'
        WRITE(6,*) ' '
        OPEN(7,FILE='SWIM7',STATUS='UNKNOWN')
        PCHECK = 0.0D0
        UCHECK = 0.0D0
        VCHECK = 0.0D0 
        CALL init
        print *," NUMBER OF POINTS IN THE X DIRECTION ",M
        print *,' NUMBER OF POINTS IN THE Y DIRECTION',N
        print *,' GRID SPACING IN THE X DIRECTION    ',DX
        print *, ' GRID SPACING IN THE Y DIRECTION    ',DY
        print *, ' TIME STEP                          ',DT
        print *, ' TIME FILTER PARAMETER              ',ALPHA
        print *, ' NUMBER OF ITERATIONS               ',ITMAX
        print *,' X BLOCK                            ',iblock
        print *,' Y BLOCK                            ',jblock
        MNMIN = MIN0(M,N)
!cycle compute: wangm
        call acemesh_runtime_init(0)
        call cpu_time(time=TIME_START)
        do NCYCLE=1,ITMAX
         CALL calc1_dag()

         CALL calc2_dag

         IF(NCYCLE .LE. 1) THEN
           CALL calc3Z
         ELSE
           CALL calc3
         ENDIF
        enddo
        call cpu_time(time=TIME_END)
        print *,"total time=",TIME_END-TIME_START
        call acemesh_runtime_shutdown()
        deallocate(U)
        deallocate(V)
        deallocate(p)
        deallocate(UNEW)
        deallocate(VNEW)
        deallocate(PNEW)
        deallocate(UOLD)
        deallocate(VOLD)
        deallocate(POLD)
        deallocate(CU)
        deallocate(CV)
        deallocate(Z)
        deallocate(PSI)
        deallocate(H)
      END PROGRAM main

      SUBROUTINE init
      use global_vars_types
      use ISO_C_BINDING
      implicit none
      INTEGER(c_int) :: I,J 
!
!      DT = 20.
!      DX = .25E5
!      DY = .25E5
!      A = 1.E6
!      ALPHA = .001
!      ITMAX = 1200
!      MPRINT = 1200
!      M = NN - 1
!      N = NM - 1
!      iblock=256
!      jblock=1024
       READ (5,*) DT
       READ (5,*) DX
       READ (5,*) DY
       READ (5,*) A
       READ (5,*) ALPHA
       READ (5,*) ITMAX
       READ (5,*) MPRINT
       READ (5,*) M
       READ (5,*) N
       READ(5,*) IBLOCK
       READ(5,*) JBLOCK
        TDT = DT
        MP1 = M+1
        NP1 = N+1

       IF (MP1 .GT. NN .OR. NP1 .GT. NM) THEN
         PRINT *, ' NP1=', NP1, ' NN=', NN
         PRINT *, ' MP1=', MP1, ' NM=', NM
         STOP
       ENDIF

      EL = N*DX
      PI = 4.D0*ATAN(1.D0)
      TPI = PI+PI
      DI = TPI/M
      DJ = TPI/N
      PCF = PI*PI*A*A/(EL*EL)
!
!     INITIAL VALUES OF THE STREAM FUNCTION AND P
!
      DO  J=1,NP1
      DO  I=1,MP1
      PSI(I,J) = A*SIN((I-.5D0)*DI)*SIN((J-.5D0)*DJ)
      P(I,J) = PCF*(COS(2.D0*(I-1)*DI)                                  &
                    +COS(2.D0*(J-1)*DJ))+50000.D0
      ENDDO
      ENDDO

      DO  J=1,N
      DO  I=1,M
      U(I+1,J) = -(PSI(I+1,J+1)-PSI(I+1,J))/DY
      V(I,J+1) = (PSI(I+1,J+1)-PSI(I,J+1))/DX
      ENDDO
      ENDDO 
!!     PERIODIC CONTINUATION

      DO J=1,N
      U(1,J) = U(M+1,J)
      V(M+1,J+1) = V(1,J+1)
      ENDDO
  
      DO I=1,M
      U(I+1,N+1) = U(I+1,1)
      V(I,1) = V(I,N+1)
      ENDDO
      U(1,N+1) = U(M+1,1)
      V(M+1,1) = V(1,N+1)
!! 12/6/00

      DO J=1,NP1
      DO I=1,MP1
      UOLD(I,J) = U(I,J)
      VOLD(I,J) = V(I,J)
      POLD(I,J) = p(I,J)
      ENDDO
      ENDDO
      return 
      END SUBROUTINE init

!calc1 is partially dag parallelized
      SUBROUTINE calc1_dag()
      !use ISO_C_BINDING
      use swim_dag_related
      implicit none
#include "aceMesh_runtime_f.h"
      INTEGER(c_int) JJ,II,I,J,jp1,ip1,topj,topi
      integer(c_int)::args_size
      type(calc1_type1) :: each
      type(calc1_type2) :: each2
      type(calc1_type3) :: each3
      integer(c_int),pointer :: nptr =>null()
      
! SPEC removed CCMIC$ DO GLOBAL
      call acemesh_begin_split_task('calc1_loop1')
      DO JJ=1,N,jblock
      DO II=1,M,iblock
         each%jj=JJ
         each%ii=II
         args_size=sizeof(each)
         if(jj+jblock<=N) then
           jp1=jj+jblock
         else
           jp1=jj
         endif

         if(ii+iblock<=M) then
           ip1=ii+iblock
         else 
           ip1=ii
         endif
         call acemesh_push_wlist1(1,c_loc(CU(II,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(CV(II,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(H(II,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(Z(II,JJ)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(p(II,JJ)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(U(II,JJ)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(V(II,JJ)),NORMAL)
         !CU(I+1,j),so which tile it will fall in? cannot decide according to local info
		 !very complex, need a algorithm....

         call acemesh_task_generator_with_neighbors(c_funloc(calc1_stencil1),  &
               c_loc(each),args_size,c_loc(nptr),c_funloc(calc1_neighbor1),c_loc(each))
         call acemesh_task_set_type(STENCIL_TASK)
      ENDDO
      ENDDO
      call acemesh_end_split_task()
!     PERIODIC CONTINUATION
      call acemesh_begin_split_task('calc1_loop2')
      DO  JJ=1,N,jblock
         each2%jj=jj
         args_size=sizeof(each2)
         if(jj+jblock<=N) then
           jp1=jj+jblock
         else 
           jp1=jj
         endif
         topi=M-mod(M,iblock)
         call acemesh_push_rlist1(1,c_loc(CU(topi,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(CU(1,JJ)),NORMAL)
         ! CV(m+1,j+1)=CV(1,j+1)
         call acemesh_push_rlist1(1,c_loc(CV(1,jj)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(CV(topi,JJ)),NORMAL)
         !H(M+1,J) = H(1,J)
         call acemesh_push_rlist1(1,c_loc(H(1,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(H(topi,JJ)),NORMAL)
         !Z(1,J+1) = Z(M+1,J+1)
         call acemesh_push_rlist1(1,c_loc(Z(topi,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(Z(1,JJ)),NORMAL)
         call acemesh_task_generator(c_funloc(calc1_stencil2),c_loc(each2), &
              args_size);
         call acemesh_task_set_type(STENCIL_TASK);
      ENDDO
      call acemesh_end_split_task()

      call acemesh_begin_split_task('calc1_loop3')
      DO II=1,M,iblock
         each3%ii=ii
         args_size=sizeof(each3)
         if(ii+iblock<=M) then
           ip1=ii+iblock
         else 
           ip1=ii
         endif
         !N+1 belongs to topi
         topj=N-mod(N,jblock) 
         topi=M-mod(M,iblock) 
         !CU(I+1,N+1) = CU(I+1,1)
         call acemesh_push_rlist1(1,c_loc(CU(ii,1)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(CU(ii,topj)),NORMAL)
         !CV(I,1) = CV(I,N+1)
         call acemesh_push_rlist1(1,c_loc(CV(ii,topj)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(CV(ii,1)),NORMAL)
         !Z(I+1,1) = Z(I+1,N+1)
         call acemesh_push_rlist1(1,c_loc(Z(ii,topj)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(Z(ii,1)),NORMAL)
         !H(I,N+1) = H(I,1)
         call acemesh_push_rlist1(1,c_loc(H(ii,1)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(H(ii,topj)),NORMAL)
         call acemesh_task_generator(c_funloc(calc1_stencil3),c_loc(each3),     &
                                                                args_size);
         call acemesh_task_set_type(STENCIL_TASK);
      ENDDO
      call acemesh_end_split_task()
      call acemesh_spawn_and_wait(99)
      
      CU(1,N+1) = CU(M+1,1)
      CV(M+1,1) = CV(1,N+1)
      Z(1,1) = Z(M+1,N+1)
      H(M+1,N+1) = H(1,1)
!       RETURN
      END SUBROUTINE calc1_dag
      
!calc2 is partially dag parallelized
!        COMPUTE NEW VALUES OF U,V,P
      SUBROUTINE calc2_dag
      use ISO_C_BINDING
      use swim_dag_related
      implicit none
#include "aceMesh_runtime_f.h"
      integer(c_int)::args_size
      type(calc1_type1) :: each
      type(calc1_type2) :: each2
      type(calc1_type3) :: each3
      integer(c_int),pointer :: nptr =>null()
      INTEGER(c_int) JJ,II,J,I,jp1,ip1,topi,topj
      call acemesh_begin_split_task('calc2_loop1')
      DO JJ=1,N,jblock
      DO II=1,M,iblock
         each%jj=JJ
         each%ii=II
         args_size=sizeof(each)
         if(jj+jblock<=N) then
           jp1=jj+jblock
         else 
           jp1=jj
         endif

         if(ii+iblock<=M) then
           ip1=ii+iblock
         else 
           ip1=ii
         endif
         !write, unew(i+1,j),vnew(i,j+1), pnew(i,j)
		 !which tile should each ref fall in? only one! can not decide using local info
		 !very complex, need a algorithm....
         call acemesh_push_wlist1(1,c_loc(UNEW(II,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(VNEW(II,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(PNEW(II,JJ)),NORMAL)
         ! read,
         !UOLD: (1,0) acording to defs in CALC3, so neighbor reads
         !VOLD:(0,1)
         call acemesh_push_rlist1(1,c_loc(UOLD(ip1,JJ)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(VOLD(II,jp1)),NORMAL)
         ! other reads fall in center
         call acemesh_push_rlist1(1,c_loc(CV(II,JJ)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(H(II,JJ)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(CU(II,JJ)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(POLD(II,JJ)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(Z(II,JJ)),NORMAL)
         call acemesh_task_generator_with_neighbors(c_funloc(calc2_stencil1),  &
              c_loc(each),args_size,c_loc(nptr),&
              c_funloc(calc2_neighbor1),c_loc(each))
         call acemesh_task_set_type(STENCIL_TASK)
      ENDDO
      ENDDO
      call acemesh_end_split_task()
!
!     PERIODIC CONTINUATION
!
      call acemesh_begin_split_task('calc2_loop2')
      DO JJ=1,N,jblock
         each2%jj=JJ
         args_size=sizeof(each2)
         if(jj+jblock<=N) then
           jp1=jj+jblock
         else 
           jp1=jj
         endif
         !N+1 blongs to topi
         topi=M-mod(M,iblock) 
         !UNEW(1,J) = UNEW(M+1,J)
         call acemesh_push_rlist1(1,c_loc(UNEW(topi,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(UNEW(1,JJ)),NORMAL)
        !vnew(M+1,J+1) = VNEW(1,J+1)
         call acemesh_push_rlist1(1,c_loc(VNEW(1,jj)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(VNEW(topi,JJ)),NORMAL)
        !PNEW(M+1,J) = PNEW(1,J)
         call acemesh_push_rlist1(1,c_loc(PNEW(1,JJ)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(PNEW(topi,JJ)),NORMAL)
         call acemesh_task_generator(c_funloc(calc2_stencil2),c_loc(each2), &
              args_size);
         call acemesh_task_set_type(STENCIL_TASK);
      ENDDO
      call acemesh_end_split_task()
      
      
      call acemesh_begin_split_task('calc2_loop3')
      DO II=1,M,iblock
         each3%ii=II
         args_size=sizeof(each3)
         if(ii+iblock<=M) then
           ip1=ii+iblock
         else 
           ip1=ii
         endif
         !N+1 blongs to topi
         topj=N-mod(N,jblock) 
         topi=M-mod(M,iblock)
         !UNEW(I+1,N+1) = UNEW(I+1,1)
         call acemesh_push_rlist1(1,c_loc(UNEW(II,1)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(UNEW(II,N+1)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(VNEW(II,topj)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(VNEW(II,1)),NORMAL)
         call acemesh_push_rlist1(1,c_loc(PNEW(II,1)),NORMAL)
         call acemesh_push_wlist1(1,c_loc(PNEW(II,topj)),NORMAL)
         call acemesh_task_generator(c_funloc(calc2_stencil3),c_loc(each3), &
              args_size);
         call acemesh_task_set_type(STENCIL_TASK);
      ENDDO
      call acemesh_end_split_task()
      call acemesh_spawn_and_wait(99)
      UNEW(1,N+1) = UNEW(M+1,1)
      VNEW(M+1,1) = VNEW(1,N+1)
      PNEW(M+1,N+1) = PNEW(1,1)
!
      RETURN
      END SUBROUTINE calc2_dag
      
      SUBROUTINE calc3Z
!         TIME SMOOTHER FOR FIRST ITERATION
      USE global_vars_types
      IMPLICIT NONE
      integer I,J
!
      TDT = TDT+TDT
      DO J=1,NP1
      DO I=1,MP1
      UOLD(I,J) = U(I,J)
      VOLD(I,J) = V(I,J)
      POLD(I,J) = p(I,J)
      U(I,J) = UNEW(I,J)
      V(I,J) = VNEW(I,J)
      p(I,J) = PNEW(I,J)
      ENDDO
      ENDDO
      RETURN
      END SUBROUTINE calc3Z

      SUBROUTINE calc3
!         TIME SMOOTHER
      use global_vars_types
      IMPLICIT NONE
      INTEGER I,J

      DO J=1,N
      DO I=1,M
      UOLD(I,J) = U(I,J)+ALPHA*(UNEW(I,J)-U(I,J)-U(I,J)+UOLD(I,J))
      VOLD(I,J) = V(I,J)+ALPHA*(VNEW(I,J)-V(I,J)-V(I,J)+VOLD(I,J))
      POLD(I,J) = p(I,J)+ALPHA*(PNEW(I,J)-p(I,J)-p(I,J)+POLD(I,J))
      U(I,J) = UNEW(I,J)
      V(I,J) = VNEW(I,J)
      p(I,J) = PNEW(I,J)
      ENDDO
      ENDDO 

!!     PERIODIC CONTINUATION

      DO J=1,N
      UOLD(M+1,J) = UOLD(1,J)
      VOLD(M+1,J) = VOLD(1,J)
      POLD(M+1,J) = POLD(1,J)
      U(M+1,J) = U(1,J)
      V(M+1,J) = V(1,J)
      p(M+1,J) = p(1,J)
      ENDDO

      DO I=1,M
      UOLD(I,N+1) = UOLD(I,1)
      VOLD(I,N+1) = VOLD(I,1)
      POLD(I,N+1) = POLD(I,1)
      U(I,N+1) = U(I,1)
      V(I,N+1) = V(I,1)
      p(I,N+1) = p(I,1)
      ENDDO

      UOLD(M+1,N+1) = UOLD(1,1)
      VOLD(M+1,N+1) = VOLD(1,1)
      POLD(M+1,N+1) = POLD(1,1)
      U(M+1,N+1) = U(1,1)
      V(M+1,N+1) = V(1,1)
      p(M+1,N+1) = p(1,1)

      RETURN
      END SUBROUTINE calc3



