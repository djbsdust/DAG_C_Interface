!---------------------------------------------------
!---------------------------------------------------
      
      subroutine timer_clear(n)

      implicit none
      integer n
      
      double precision start(64), elapsed(64)
      common /tt/ start, elapsed

      elapsed(n) = 0.0
      return
      end


!---------------------------------------------------
!---------------------------------------------------

      subroutine timer_start(n)

      implicit none
      external         elapsed_time
      double precision elapsed_time
      integer n
      double precision start(64), elapsed(64)
      common /tt/ start, elapsed

      start(n) = elapsed_time()

      return
      end
      

!--------------------------------------------------
!--------------------------------------------------

      subroutine timer_stop(n)

      implicit none
      external         elapsed_time
      double precision elapsed_time
      integer n
      double precision start(64), elapsed(64)
      common /tt/ start, elapsed
      double precision t, now
      now = elapsed_time()
      t = now - start(n)
      elapsed(n) = elapsed(n) + t

      return
      end


!------------------------------------------------
!------------------------------------------------

      double precision function timer_read(n)

      implicit none
      integer n
      double precision start(64), elapsed(64)
      common /tt/ start, elapsed
      
      timer_read = elapsed(n)
      return
      end


!-----------------------------------------------
!-----------------------------------------------

      double precision function elapsed_time()

      implicit none
!$    external         omp_get_wtime
!$    double precision omp_get_wtime

      real, dimension(2) :: vals
      real etime
      real :: t


! This function must measure wall clock time, not CPU time. 
! Since there is no portable timer in Fortran (77)
! we call a routine compiled in C (though the C source may have
! to be tweaked). 
      
       t=etime(vals)
! The following is not ok for "official" results because it reports
! CPU time not wall clock time. It may be useful for developing/testing
! on timeshared Crays, though. 
!        call second(t)

      elapsed_time = t

      return
      end

