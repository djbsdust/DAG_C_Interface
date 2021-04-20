!!ifort -O3 -o omp  omp-swim.f -free -cpp -openmp
!! export OMP_NUM_THREADS=8
!! ./omp <input
#define NN 3802
#define NM 3802  
      MODULE globals
      USE ISO_C_BINDING
      IMPLICIT NONE
      real(c_double),allocatable :: U(:,:)
      real(c_double),allocatable :: V(:,:)
      real(c_double),allocatable :: P(:,:)    
      real(c_double),allocatable ::UNEW(:,:)
      real(c_double),allocatable :: VNEW(:,:)
      real(c_double),allocatable ::PNEW(:,:)    
      real(c_double),allocatable :: UOLD(:,:)
      real(c_double),allocatable :: VOLD(:,:) 
      real(c_double),allocatable :: POLD(:,:)    
      real(c_double),allocatable :: CU(:,:) 
      real(c_double),allocatable :: CV(:,:) 
      real(c_double),allocatable :: Z(:,:)    
      real(c_double),allocatable :: H(:,:)
      real(c_double),allocatable :: PSI(:,:)
      REAL(c_double)  :: DT,TDT,DX,DY,A,ALPHA,EL,PI,TPI,DI,DJ,PCF
      INTEGER(c_int) :: ITMAX,MPRINT,M,N,MP1,NP1,JBLOCK,IBLOCK,MNMIN,num_thread
      real(c_double)  :: U1(NN,NM), V1(NN,NM), P1(NN,NM),                   &
                   UNEW1(NN,NM), VNEW1(NN,NM),PNEW1(NN,NM),            &
                   UOLD1(NN,NM),VOLD1(NN,NM), POLD1(NN,NM),            &
                   CU1(NN,NM), CV1(NN,NM), Z1(NN,NM),                  &
                   H1(NN,NM), PSI1(NN,NM)
      real(c_double)  :: DT1,TDT1,DX1,DY1,A1,ALPHA1,EL1,PI1,TPI1,DI1,DJ1,PCF1
      integer(c_double) :: ITMAX1,MPRINT1,M1,N1,MP11,NP11,JBLOCK1,IBLOCK1,   &
                            MNMIN1
      real(c_double)PCHECK,UCHECK,VCHECK
      END MODULE globals


      PROGRAM main 
      USE globals
      use omp_lib
      IMPLICIT NONE
      !character(3) OMP_NUM_THREADS
      INTEGER I,J,NCYCLE,JCHECK,ICHECK
      REAL*8 TIME_START,TIME_END
      allocate(U(NN,NM))
      allocate(V(NN,NM))
      allocate(P(NN,NM))
      allocate(UNEW(NN,NM))
      allocate(VNEW(NN,NM))
      allocate(PNEW(NN,NM))
      allocate(UOLD(NN,NM))
      allocate(VOLD(NN,NM))
      allocate(POLD(NN,NM))
      allocate(CU(NN,NM))
      allocate(CV(NN,NM))
      allocate(Z(NN,NM))
      allocate(H(NN,NM))
      allocate(PSI(NN,NM))
      !num_thread=1
      WRITE(6,*) ' SPEC benchmark 171.swim'
      WRITE(6,*) ' '
      OPEN(7,FILE='SWIM7',STATUS='UNKNOWN')
      !call get_environment_variable("OMP_NUM_THREADS",OMP_NUM_THREADS)
      !read (OMP_NUM_THREADS,'(I3)') num_thread
      PCHECK = 0.0D0
      UCHECK = 0.0D0
      VCHECK = 0.0D0       
      CALL init

      print *," NUMBER OF POINTS IN THE X DIRECTION ",M
      print *,' NUMBER OF POINTS IN THE Y DIRECTION',N
      print *,' GRID SPACING IN THE X DIRECTION    ',DX
      print *, ' GRID SPACING IN THE Y DIRECTION    ',DY
      print *, ' TIME STEP                    ',DT
      print *, ' TIME FILTER PARAMETER            ',ALPHA
      print *, ' NUMBER OF ITERATIONS             ',ITMAX
      print *,' X BLOCK                      ',IBLOCK
      print *,' Y BLOCK                      ',JBLOCK
      MNMIN = MIN0(M,N)
      TIME_START=OMP_get_wtime()
      do NCYCLE=1,ITMAX
      CALL calc1

      CALL calc2

      IF(NCYCLE .LE. 1) THEN
         CALL calc3Z
      ELSE
         CALL calc3
      ENDIF
      enddo
      TIME_END=OMP_get_wtime()
      print *,"total time=",TIME_END-TIME_START
      deallocate(U)
      deallocate(V)
      deallocate(P)
      deallocate(UNEW)
      deallocate(VNEW)
      deallocate(PNEW)
      deallocate(UOLD)
      deallocate(VOLD)
      deallocate(POLD)
      deallocate(CU)
      deallocate(CV)
      deallocate(Z)
      deallocate(H)
      deallocate(PSI)
      END PROGRAM main

      SUBROUTINE init
      USE globals
      implicit none
      INTEGER :: I,J 
!      DT = 20.

!      DX = .25E5
!      DY = .25E5
!      A = 1.E6
!      ALPHA = .001
!      ITMAX = 1200
!      MPRINT = 1200
!      M = NN - 1
!      N = NM - 1
!      JBLOCK=1024
!      IBLOCK=1024
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
      IF (MP1 > NN .OR. NP1 > NM) THEN
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
      POLD(I,J) = P(I,J)
      ENDDO
      ENDDO
!        END OF INITIALIZATION
      RETURN
      END SUBROUTINE init

! SPEC removed CCMIC$ MICRO
      SUBROUTINE calc1
      use globals
      use omp_lib
      implicit none
      real*8 FSDX,FSDY
      INTEGER JJ,II,J,I,k,l
      FSDX = 4.D0/DX
      FSDY = 4.D0/DY
! SPEC removed CCMIC$ DO GLOBAL         
!$omp parallel 
!$omp do  
!      k=OMP_get_thread_num()
      DO JJ=1,N,JBLOCK
      DO II=1,M,IBLOCK
      DO J=JJ,MIN(N,JJ+JBLOCK)
      DO I=II,MIN(M,II+IBLOCK)
      CU(I+1,J) = .5D0*(P(I+1,J)+P(I,J))*U(I+1,J)
      CV(I,J+1) = .5D0*(P(I,J+1)+P(I,J))*V(I,J+1)
      Z(I+1,J+1) = (FSDX*(V(I+1,J+1)-V(I,J+1))-FSDY*(U(I+1,J+1)         &
               -U(I+1,J)))/(P(I,J)+P(I+1,J)+P(I+1,J+1)+P(I,J+1))
      H(I,J) = P(I,J)+.25D0*(U(I+1,J)*U(I+1,J)+U(I,J)*U(I,J)            &
                     +V(I,J+1)*V(I,J+1)+V(I,J)*V(I,J))
      ENDDO
      ENDDO
      ENDDO
      ENDDO

!$omp do  
      DO  JJ=1,N,JBLOCK
      DO  J=JJ,MIN(N,JJ+JBLOCK)
      CU(1,J) = CU(M+1,J)
      CV(M+1,J+1) = CV(1,J+1)
      Z(1,J+1) = Z(M+1,J+1)
      H(M+1,J) = H(1,J)
      ENDDO
      ENDDO

!$omp do  
      DO II=1,M,IBLOCK
      DO I=II,MIN(M,II+IBLOCK)
      CU(I+1,N+1) = CU(I+1,1)
      CV(I,1) = CV(I,N+1)
      Z(I+1,1) = Z(I+1,N+1)
      H(I,N+1) = H(I,1)
      ENDDO
      ENDDO
!$omp end parallel 
      CU(1,N+1) = CU(M+1,1)
      CV(M+1,1) = CV(1,N+1)
      Z(1,1) = Z(M+1,N+1)
      H(M+1,N+1) = H(1,1)
!        END OF LOOP 100 CALCULATIONS
      RETURN
      END 

! SPEC removed CCMIC$ MICRO
      SUBROUTINE calc2
!        COMPUTE NEW VALUES OF U,V,P
      use globals
      implicit none
      real*8 TDTS8,TDTSDX,TDTSDY
      INTEGER JJ,II,J,I
      TDTS8 = TDT/8.D0
      TDTSDX = TDT/DX
      TDTSDY = TDT/DY
! SPEC removed CCMIC$ DO GLOBAL
!$omp parallel 
!$omp do  
      DO JJ=1,N,JBLOCK
      DO II=1,M,IBLOCK
      DO J=JJ,MIN(N,JJ+JBLOCK)
      DO I=II,MIN(M,II+IBLOCK)
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
      ENDDO
      ENDDO

!     PERIODIC CONTINUATION
!$omp do  
      DO JJ=1,N,JBLOCK
      DO J=JJ,MIN(N,JJ+JBLOCK)
      UNEW(1,J) = UNEW(M+1,J)
      VNEW(M+1,J+1) = VNEW(1,J+1)
      PNEW(M+1,J) = PNEW(1,J)
      ENDDO
      ENDDO

!$omp do  
      DO II=1,M,IBLOCK
      DO I=II,MIN(M,II+IBLOCK)
      UNEW(I+1,N+1) = UNEW(I+1,1)
      VNEW(I,1) = VNEW(I,N+1)
      PNEW(I,N+1) = PNEW(I,1)
      ENDDO
      ENDDO
!$omp end parallel
      UNEW(1,N+1) = UNEW(M+1,1)
      VNEW(M+1,1) = VNEW(1,N+1)
      PNEW(M+1,N+1) = PNEW(1,1)
!
      RETURN
      END 

      SUBROUTINE calc3Z
!         TIME SMOOTHER FOR FIRST ITERATION
      USE globals
      IMPLICIT NONE
      integer I,J
      TDT = TDT+TDT
      DO J=1,NP1
      DO I=1,MP1
      UOLD(I,J) = U(I,J)
      VOLD(I,J) = V(I,J)
      POLD(I,J) = P(I,J)
      U(I,J) = UNEW(I,J)
      V(I,J) = VNEW(I,J)
      P(I,J) = PNEW(I,J)
      ENDDO
      ENDDO
      RETURN
      END
! SPEC removed CCMIC$ MICRO
      SUBROUTINE calc3
      USE globals
      IMPLICIT NONE

      INTEGER I,J
!! SPEC removed CCMIC$ DO GLOBAL
      DO J=1,N
      DO I=1,M
      UOLD(I,J) = U(I,J)+ALPHA*(UNEW(I,J)-U(I,J)-U(I,J)+UOLD(I,J))
      VOLD(I,J) = V(I,J)+ALPHA*(VNEW(I,J)-V(I,J)-V(I,J)+VOLD(I,J))
      POLD(I,J) = P(I,J)+ALPHA*(PNEW(I,J)-P(I,J)-P(I,J)+POLD(I,J))
      U(I,J) = UNEW(I,J)
      V(I,J) = VNEW(I,J)
      P(I,J) = PNEW(I,J)
      ENDDO
      ENDDO 

!!     PERIODIC CONTINUATION

      DO J=1,N
      UOLD(M+1,J) = UOLD(1,J)
      VOLD(M+1,J) = VOLD(1,J)
      POLD(M+1,J) = POLD(1,J)
      U(M+1,J) = U(1,J)
      V(M+1,J) = V(1,J)
      P(M+1,J) = P(1,J)
      ENDDO

      DO I=1,M
      UOLD(I,N+1) = UOLD(I,1)
      VOLD(I,N+1) = VOLD(I,1)
      POLD(I,N+1) = POLD(I,1)
      U(I,N+1) = U(I,1)
      V(I,N+1) = V(I,1)
      P(I,N+1) = P(I,1)
      ENDDO

      UOLD(M+1,N+1) = UOLD(1,1)
      VOLD(M+1,N+1) = VOLD(1,1)
      POLD(M+1,N+1) = POLD(1,1)
      U(M+1,N+1) = U(1,1)
      V(M+1,N+1) = V(1,1)
      P(M+1,N+1) = P(1,1)

      RETURN
      END

