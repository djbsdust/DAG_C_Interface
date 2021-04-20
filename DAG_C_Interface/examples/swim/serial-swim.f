!!ifort -O -o serial serial-swim.f -free -cpp
!!./serial <input
#define N1 3802
#define N2 3802  
!********************************************
!  i get rid of 'common ',for read convenient,
!and decrease write codes
!
!********************************************    
	MODULE DAG
        USE ISO_C_BINDING
	IMPLICIT NONE
   	
	REAL  :: U(N1,N2), V(N1,N2), P(N1,N2),                        &
	             UNEW(N1,N2), VNEW(N1,N2),PNEW(N1,N2),                     &
	             UOLD(N1,N2),VOLD(N1,N2), POLD(N1,N2),                     &
	             CU(N1,N2), CV(N1,N2), Z(N1,N2),                           & 
	             H(N1,N2), PSI(N1,N2)
	REAL  :: DT,TDT,DX,DY,A,ALPHA,EL,PI,TPI,DI,DJ,PCF
	INTEGER :: ITMAX,MPRINT,M,N,MP1,NP1,JBLOCK,IBLOCK,MNMIN
	
	END MODULE DAG


      PROGRAM main 
	USE DAG
	IMPLICIT NONE
	INTEGER I,J,NCYCLE,JCHECK,ICHECK
	REAL*8 TIME_START,TIME_END,PCHECK,UCHECK,VCHECK
!      IMPLICIT INTEGER	(I-N)
!      IMPLICIT REAL*8	(A-H, O-Z)
!       PARAMETER (N1=3802, N2=3802)


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
        print *,' X BLOCK                            ',IBLOCK
        print *,' Y BLOCK                            ',JBLOCK
      MNMIN = MIN0(M,N)
!	IBLOCK=1024
!	JBLOCK=1024
!	call cpu_time(time=TIME1)
!************************************************!
!wangm:2016-3-29  
!i rebuild blow codes ,which are cancled.rebuild
!for reading convenence and cancel to use 'go to'.
!have keep right of result-'PCHECK ,VCHECK ,UCHECK ' 
!
!************************************************
!      TIME = 0
!      NCYCLE = 0
!   90 NCYCLE = NCYCLE + 1
!      CALL calc1
!      CALL calc2
!      TIME = TIME + DT
!      IF(MOD(NCYCLE,MPRINT) .NE. 0) GO TO 370
!      PTIME = TIME/3600.
!      print *,"??????",MPRINT
!      WRITE(7,350) NCYCLE,PTIME
!  350 FORMAT(/' CYCLE NUMBER',I5,' MODEL TIME IN  HOURS', F6.2)
!      WRITE(7,360) (UNEW(I,I),I=1,MNMIN,10)
!  360 FORMAT(/' DIAGONAL ELEMENTS OF U ', //(8E15.7))
!        PCHECK = 0.0D0
!        UCHECK = 0.0D0
!        VCHECK = 0.0D0       
!     !   DO   JCHECK = 1, MNMIN
!     !   DO  ICHECK = 1, MNMIN
!	DO  J=1,MNMIN,JBLOCK
!        DO   JCHECK = J, MIN(MNMIN,J+JBLOCK)
!	DO  I=1,MNMIN,IBLOCK
!        DO  ICHECK = I, MIN(MNMIN,I+IBLOCK)
!         PCHECK = PCHECK + ABS(PNEW(ICHECK,JCHECK))
!         UCHECK = UCHECK + ABS(UNEW(ICHECK,JCHECK))
!         VCHECK = VCHECK + ABS(VNEW(ICHECK,JCHECK))
!	ENDDO
!	ENDDO
!	UNEW(JCHECK,JCHECK) = UNEW(JCHECK,JCHECK) 
!     1  * ( MOD (JCHECK, 400) /400.)
!	ENDDO
!	ENDDO
!        WRITE(6,366) PCHECK, UCHECK, VCHECK
! 366    FORMAT(/,
!     *         ' Pcheck = ',E12.4,/,
!     *         ' Ucheck = ',E12.4,/,
!     *         ' Vcheck = ',E12.4,/)
!
!370   CONTINUE
!      IF(NCYCLE .GE. ITMAX) THEN
!!	call cpu_time(time=TIME2)
!	print *,"time=",TIME2-TIME1
!	call timer_stop(1)
!	tt=timer_read(1)
!	print *,"total time=",tt
!      STOP
!      ENDIF
!     IF(NCYCLE .LE. 1) THEN
!         CALL calc3Z
!      ELSE
!         CALL calc3
!      ENDIF

!       GO TO 90
!cycle compute: wangm
	call cpu_time(time=TIME_START)
      do NCYCLE=1,ITMAX
      CALL calc1

      CALL calc2

      IF(NCYCLE .LE. 1) THEN
         CALL calc3Z
      ELSE
         CALL calc3
      ENDIF
      enddo
	call cpu_time(time=TIME_END)
        print *,"total time=",TIME_END-TIME_START
! test check: wangm
        DO  J=1,MNMIN,JBLOCK
        DO  JCHECK = J, MIN(MNMIN,J+JBLOCK)
        DO  I=1,MNMIN,IBLOCK
        DO  ICHECK = I, MIN(MNMIN,I+IBLOCK)
         PCHECK = PCHECK + ABS(PNEW(ICHECK,JCHECK))
         UCHECK = UCHECK + ABS(UNEW(ICHECK,JCHECK))
         VCHECK = VCHECK + ABS(VNEW(ICHECK,JCHECK))
        ENDDO
        ENDDO
        UNEW(JCHECK,JCHECK) = UNEW(JCHECK,JCHECK)*(MOD(JCHECK,400)/400.)
        ENDDO
        ENDDO
       
        WRITE(6,366) PCHECK, UCHECK, VCHECK
 366    FORMAT(/,'Pcheck = ',E12.4,/,'Ucheck = ',E12.4,/,'Vcheck = ',E12.4,/)
      END PROGRAM main

      SUBROUTINE init
	USE DAG
	implicit none
	INTEGER :: I,J 
!      DT = 20.

!      DX = .25E5
!      DY = .25E5
!      A = 1.E6
!      ALPHA = .001
!      ITMAX = 1200
!      MPRINT = 1200
!      M = N1 - 1
!      N = N2 - 1
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
     IF (MP1 > N1 .OR. NP1 > N2) THEN
        PRINT *, ' NP1=', NP1, ' N1=', N1
        PRINT *, ' MP1=', MP1, ' N2=', N2
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
	use dag
	implicit none
!      IMPLICIT REAL*8	(A-H, O-Z)
!      PARAMETER (N1=3802, N2=3802)

!      COMMON  U(N1,N2), V(N1,N2), P(N1,N2),
!     *        UNEW(N1,N2), VNEW(N1,N2),
!     1        PNEW(N1,N2), UOLD(N1,N2),
!     *        VOLD(N1,N2), POLD(N1,N2),
!     2        CU(N1,N2), CV(N1,N2),
!     *        Z(N1,N2), H(N1,N2), PSI(N1,N2)

!      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
!     1              NP1,EL,PI,TPI,DI,DJ,PCF,IBLOCK,JBLOCK
	real*8 FSDX,FSDY
	INTEGER JJ,II,J,I
      FSDX = 4.D0/DX
      FSDY = 4.D0/DY
!	IBLOCK=1024
!	JBLOCK=1024
! SPEC removed CCMIC$ DO GLOBAL
!      DO  J=1,N
!      DO  I=1,M
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
!     PERIODIC CONTINUATION
! 	print *,"N=",N,"M=",M
      DO  JJ=1,N,JBLOCK
      DO  J=JJ,MIN(N,JJ+JBLOCK)
      CU(1,J) = CU(M+1,J)
      CV(M+1,J+1) = CV(1,J+1)
      Z(1,J+1) = Z(M+1,J+1)
      H(M+1,J) = H(1,J)
      ENDDO
	ENDDO

      DO II=1,M,IBLOCK
      DO I=II,MIN(M,II+IBLOCK)
      CU(I+1,N+1) = CU(I+1,1)
      CV(I,1) = CV(I,N+1)
      Z(I+1,1) = Z(I+1,N+1)
      H(I,N+1) = H(I,1)
      ENDDO
	ENDDO
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
	use dag
	implicit none
!      IMPLICIT REAL*8	(A-H, O-Z)
!      PARAMETER (N1=3802, N2=3802)

!      COMMON  U(N1,N2), V(N1,N2), P(N1,N2),
!     *        UNEW(N1,N2), VNEW(N1,N2),
!     1        PNEW(N1,N2), UOLD(N1,N2),
!     *        VOLD(N1,N2), POLD(N1,N2),
!     2        CU(N1,N2), CV(N1,N2),
!     *        Z(N1,N2), H(N1,N2), PSI(N1,N2)

!      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
!     1              NP1,EL,PI,TPI,DI,DJ,PCF,IBLOCK,JBLOCK
	real*8 TDTS8,TDTSDX,TDTSDY
	INTEGER JJ,II,J,I
      TDTS8 = TDT/8.D0
      TDTSDX = TDT/DX
      TDTSDY = TDT/DY
!	IBLOCK=1024
!	JBLOCK=1024
! SPEC removed CCMIC$ DO GLOBAL
!      DO J=1,N
!      DO I=1,M
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
!
!     PERIODIC CONTINUATION
!
	DO JJ=1,N,JBLOCK
      DO J=JJ,MIN(N,JJ+JBLOCK)
      UNEW(1,J) = UNEW(M+1,J)
      VNEW(M+1,J+1) = VNEW(1,J+1)
      PNEW(M+1,J) = PNEW(1,J)
      ENDDO
	ENDDO
	DO II=1,M,IBLOCK
      DO I=II,MIN(M,II+IBLOCK)
      UNEW(I+1,N+1) = UNEW(I+1,1)
      VNEW(I,1) = VNEW(I,N+1)
      PNEW(I,N+1) = PNEW(I,1)
      ENDDO
	ENDDO
      UNEW(1,N+1) = UNEW(M+1,1)
      VNEW(M+1,1) = VNEW(1,N+1)
      PNEW(M+1,N+1) = PNEW(1,1)
!
      RETURN
      END 

      SUBROUTINE calc3Z

!         TIME SMOOTHER FOR FIRST ITERATION
	USE DAG
	IMPLICIT NONE
!      IMPLICIT REAL*8	(A-H, O-Z)
!      PARAMETER (N1=3802, N2=3802)

!      COMMON  U(N1,N2), V(N1,N2), P(N1,N2),
!     *        UNEW(N1,N2), VNEW(N1,N2),
!     1        PNEW(N1,N2), UOLD(N1,N2),
!     *        VOLD(N1,N2), POLD(N1,N2),
!     2        CU(N1,N2), CV(N1,N2),
!     *        Z(N1,N2), H(N1,N2), PSI(N1,N2)
!
!      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
!     1              NP1,EL,PI,TPI,DI,DJ,PCF
!
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
	USE DAG
	IMPLICIT NONE
!         TIME SMOOTHER

!      IMPLICIT REAL*8	(A-H, O-Z)
!      PARAMETER (N1=3802, N2=3802)

!      COMMON  U(N1,N2), V(N1,N2), P(N1,N2),
!     *        UNEW(N1,N2), VNEW(N1,N2),
!     1        PNEW(N1,N2), UOLD(N1,N2),
!     *        VOLD(N1,N2), POLD(N1,N2),
!     2        CU(N1,N2), CV(N1,N2),
!     *        Z(N1,N2), H(N1,N2), PSI(N1,N2)

!      COMMON /CONS/ DT,TDT,DX,DY,A,ALPHA,ITMAX,MPRINT,M,N,MP1,
!     1              NP1,EL,PI,TPI,DI,DJ,PCF

!!        TIME SMOOTHING AND UPDATE FOR NEXT CYCLE

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

