    
     module global_vars_types
     use ISO_C_BINDING
     implicit none
     integer,parameter :: NN=3802,NM=3802

     real(c_double),allocatable :: U(:,:)
     real(c_double),allocatable :: V(:,:)
     real(c_double),allocatable :: p(:,:)                    
     real(c_double),allocatable ::UNEW(:,:)
     real(c_double),allocatable :: VNEW(:,:)
     real(c_double),allocatable ::PNEW(:,:)        
     real(c_double),allocatable :: UOLD(:,:)
     real(c_double),allocatable :: VOLD(:,:) 
     real(c_double),allocatable :: POLD(:,:)           
     real(c_double),allocatable :: CU(:,:) 
     real(c_double),allocatable :: CV(:,:) 

     real(c_double),allocatable :: PSI(:,:)
     real(c_double),allocatable :: Z(:,:)         
     real(c_double),allocatable :: H(:,:)
     real(c_double)  :: DT,TDT,DX,DY,A,ALPHA,EL,PI,TPI,DI,DJ,PCF
     INTEGER(c_int) :: ITMAX,MPRINT,M,N,MP1,NP1,jblock,iblock,MNMIN
     real(c_double)  :: U1(NN,NM), V1(NN,NM), P1(NN,NM),             &
                 UNEW1(NN,NM), VNEW1(NN,NM),PNEW1(NN,NM),            &
                 UOLD1(NN,NM),VOLD1(NN,NM), POLD1(NN,NM),            &
                 CU1(NN,NM), CV1(NN,NM), Z1(NN,NM),                  &
                 H1(NN,NM), PSI1(NN,NM)
     real(c_double)  :: DT1,TDT1,DX1,DY1,A1,ALPHA1,EL1,PI1,TPI1,DI1,DJ1,PCF1
     integer(c_double) :: ITMAX1,MPRINT1,M1,N1,MP11,NP11,JBLOCK1,IBLOCK1,   &
                          MNMIN1
     real(c_double)PCHECK,UCHECK,VCHECK

     
     type calc1_type1
       integer(c_int) ii,jj
     end type calc1_type1

     type calc1_type2
       integer(c_int) jj
     end type calc1_type2

     type calc1_type3
       integer(c_int) ii
     end type calc1_type3


     end module global_vars_types

     module swim_dag_related
     USE global_vars_types
     use dag_struct

     contains
    
!---------------------------calc1 loop1---------------------

    SUBROUTINE calc1_stencil1(args)
    use ISO_C_BINDING
    implicit none
    
    type(calc1_type1),target ::args
    integer(c_int):: JJ,II,J,I
    real*8 FSDX,FSDY
    FSDX = 4.D0/DX
    FSDY = 4.D0/DY
    II= args%ii
    JJ=args%jj
    DO J=JJ,MIN(N,JJ+jblock)
    DO I=II,MIN(M,II+iblock)
      CU(I+1,J) = .5D0*(p(I+1,J)+p(I,J))*U(I+1,J)
      CV(I,J+1) = .5D0*(p(I,J+1)+p(I,J))*V(I,J+1)
      Z(I+1,J+1) = (FSDX*(V(I+1,J+1)-V(I,J+1))-FSDY*(U(I+1,J+1)         &
                -U(I+1,J)))/(p(I,J)+p(I+1,J)+p(I+1,J+1)+p(I,J+1))
      H(I,J) = p(I,J)+.25D0*(U(I+1,J)*U(I+1,J)+U(I,J)*U(I,J)            &
                    +V(I,J+1)*V(I,J+1)+V(I,J)*V(I,J))
    ENDDO
    ENDDO
    return
    END SUBROUTINE calc1_stencil1
    
!---------------------------calc1 neigh1---------------------
! read neighbors:
!P(,,): (0,1),(1,0),(1,1),(0,0)
!V(,,):(0,1),(1,1),(0,0)
!U(,,):(1,0),(1,1),(0,0)

! but the reaching defs are in calc3
! calc3-loop write: uold(i,j),vold(i,j),pold(i,j),u(i,j),v(i,j),p(i,j)

!So we set neighbors according to array P, if P's neighbor satisfied
! then V and U 's neighbors are also satified

    SUBROUTINE calc1_neighbor1(neighbor_addrs,args)
    use ISO_C_BINDING
    use global_vars_types
    use dag_struct
    IMPLICIT NONE
#include "aceMesh_runtime_f.h"
    real(c_double) src,src1
    type(calc1_type1),target :: args
    integer(c_int) ii,jj,size,num
    type(ptr_array) ::neighbor_addrs
    type(c_ptr),pointer::array(:)
    size=MAX_NEIGHBORS

    call c_f_pointer(neighbor_addrs%arr,array,[size])
    ii=args%ii
    jj=args%jj
    num=0
    if(ii+iblock<=M)then
      num=num+1
      array(num)=c_loc(p(ii+iblock,jj))
    endif
    if(jj+jblock<=N)then
      num=num+1
      array(num)=c_loc(p(ii,jj+jblock))
      if(ii+iblock<=M)then
         num=num+1
         array(num)=c_loc(p(ii+iblock,jj+jblock))
      endif
    endif
    neighbor_addrs%len=num
    RETURN  
    END SUBROUTINE calc1_neighbor1
    
!---------------------------calc1 loop2---------------------
    subroutine calc1_stencil2(args)
    !use ISO_C_BINDING
    use global_vars_types
    implicit none
    type(calc1_type2),target ::args
    integer(c_int) :: jj,j
    jj=args%jj
    DO  J=JJ,MIN(N,JJ+jblock)
         CU(1,J) = CU(M+1,J)
         CV(M+1,J+1) = CV(1,J+1)
         Z(1,J+1) = Z(M+1,J+1)
         H(M+1,J) = H(1,J)
    ENDDO
    return
    end subroutine calc1_stencil2
    
!---------------------------calc1 loop3---------------------
    subroutine calc1_stencil3(args)
    ! use ISO_C_BINDING
     use global_vars_types
    !implicit none
    type(calc1_type3),target ::args
    integer(c_int) ::I,II
    II=args%ii
    DO I=II,MIN(M,II+iblock)
         CU(I+1,N+1) = CU(I+1,1)
         CV(I,1) = CV(I,N+1)
         Z(I+1,1) = Z(I+1,N+1)
         H(I,N+1) = H(I,1)
    ENDDO
    return
    end subroutine calc1_stencil3
    
!---------------------------calc2 loop1---------------------
    SUBROUTINE calc2_stencil1(args)
    !use ISO_C_BINDING
        USE global_vars_types
    !implicit none
    
    type(calc1_type1),target ::args
    integer(c_int):: JJ,II,J,I
    real(c_double) TDTS8,TDTSDX,TDTSDY
    TDTS8 = TDT/8.D0
    TDTSDX = TDT/DX
    TDTSDY = TDT/DY
    jj=args%jj
    ii=args%ii
    DO J=JJ,MIN(N,JJ+jblock)
    DO I=II,MIN(M,II+iblock)
        UNEW(I+1,J) = UOLD(I+1,J)+                                        &
         TDTS8*(Z(I+1,J+1)+Z(I+1,J))*(CV(I+1,J+1)+CV(I,J+1)+CV(I,J)     &
            +CV(I+1,J))-TDTSDX*(H(I+1,J)-H(I,J))         
        VNEW(I,J+1) = VOLD(I,J+1)-TDTS8*(Z(I+1,J+1)+Z(I,J+1))             &
            *(CU(I+1,J+1)+CU(I,J+1)+CU(I,J)+CU(I+1,J))                 &
             -TDTSDY*(H(I,J+1)-H(I,J))
        PNEW(I,J) = POLD(I,J)-TDTSDX*(CU(I+1,J)-CU(I,J))                  &
            -TDTSDY*(CV(I,J+1)-CV(I,J))
    ENDDO
    ENDDO
    return
    end subroutine calc2_stencil1
    
!---------------------------calc2 neigh1---------------------
! read neighbors
! Z,CV,CU: (1,1),(0,1),(1,0),(0,0)
! H: (1,0), (0,1)
! but reaching defs are in calc1
! calc1: 
!       writes CU(i+1,j),CV(i,j+1),Z(i+1,j+1)
!
! so CU's addr: center,(0,1),(-1,0),(-1,-1)
!    CV's addr: center,(1,0),(1,-1),(0,-1)
!    Z's  addr: center,(-1,0),(0,-1),(-1,-1)
!
     SUBROUTINE calc2_neighbor1(neighbor_addrs,args)
     use ISO_C_BINDING
     use global_vars_types
     !IMPLICIT NONE
#include "aceMesh_runtime_f.h"
     
     type(calc1_type1),target :: args
     integer(c_int) ii,jj,size,num,ip1,jp1,im1,jm1
     type(ptr_array) ::neighbor_addrs
     type(c_ptr),pointer::array(:)
     size=MAX_NEIGHBORS
     call c_f_pointer(neighbor_addrs%arr,array,[size])
     ii=args%ii
     jj=args%jj
     num=0
	 !plus 1
     if(ii+iblock<=M)then
	   ip1=ii+iblock
	 else 
	   ip1=ii
	 endif
     if(jj+jblock<=N) then 
	   jp1=jj+jblock
	 else
	   jp1=jj
	 endif
	 !minus 1
     if(ii-iblock>=1)then
	   im1=ii-iblock
	 else 
	   im1=ii
	 endif
     if(jj-jblock>=1) then 
	   jm1=jj-jblock
	 else
	   jm1=jj
	 endif
     !CU's addr: center,(0,1),(-1,0),(-1,-1)
	 ! the following may coincide with center
       num=num+1
       array(num)=c_loc(CU(ii,jp1))
       num=num+1
       array(num)=c_loc(CU(im1,jj))
       num=num+1
       array(num)=c_loc(CU(im1,jm1))
     !CV's addr: center,(1,0),(1,-1),(0,-1)
       num=num+1
       array(num)=c_loc(CV(ip1,jj))
       num=num+1
       array(num)=c_loc(CV(ip1,jm1))
       num=num+1
       array(num)=c_loc(CV(ii,jm1))
     !Z's  addr: center,(-1,0),(0,-1),(-1,-1)
	 !no need to add, since their dep edges are coincide with the previous
	 ! CU/CV/P are written in the same loop

     !no need to specify H, since no definition in the time integration loop
	 neighbor_addrs%len=num
     RETURN  
     END SUBROUTINE calc2_neighbor1

!--------------------calc2 loop2--------------------

    subroutine calc2_stencil2(args)
    !use ISO_C_BINDING
    use global_vars_types
    !implicit none
    type(calc1_type2),target ::args
    integer(c_int) :: jj,j
    jj=args%jj
    DO J=JJ,MIN(N,JJ+jblock)
          UNEW(1,J) = UNEW(M+1,J)
          VNEW(M+1,J+1) = VNEW(1,J+1)
          PNEW(M+1,J) = PNEW(1,J)
    ENDDO
    return
    end subroutine calc2_stencil2

!--------------------calc2 loop3--------------------
    subroutine calc2_stencil3(args)
     !use ISO_C_BINDING
     use global_vars_types
    !implicit none
    type(calc1_type3),target ::args
    integer(c_int) :: ii,i
    ii=args%ii
    DO I=II,MIN(M,II+iblock)
         UNEW(I+1,N+1) = UNEW(I+1,1)
         VNEW(I,1) = VNEW(I,N+1)
         PNEW(I,N+1) = PNEW(I,1)
    ENDDO
    return
    end subroutine calc2_stencil3
    end module swim_dag_related    
