MIC = 
ifeq ($(MIC),TRUE)
CXX=icc -mmic -O3
CFLAGS =  -std=c++0x -Wall  ${BLAS_LIB} -I${SCHEDULER_INSTALL_PATH}/include -L${SCHEDULER_INSTALL_PATH}/lib -L/opt/intel/tbb42/lib/mic/  -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -lpthread -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -lrt #-L/opt/intel/tbb42/lib/mic/
else
CXX=icc  -O3
CFLAGS =  -I${SCHEDULER_INSTALL_PATH}/include -L${SCHEDULER_INSTALL_PATH}/lib
endif

SCHEDULER_INSTALL_PATH=${DAG_HOME}
BLAS_LIB= -I/opt/intel/mkl/include -L/opt/intel/mkl/lib/mic

DAG_LIBFLAGS = -ltbb -ldag_task_scheduler  -lm -lmkl_intel_lp64 -lmkl_sequential -lmkl_core #-lopenblas -lm 
LIBFLAGS = -ltbb  -lm  -lmkl_intel_lp64 -lmkl_sequential -lmkl_core  #-lopenblas -lm 

all:  dag omp

dag:
	$(CXX) $(CFLAGS) -o cholesky_dag cholesky_dag.cpp $(DAG_LIBFLAGS)

serial:
	$(CXX) $(CFLAGS) -o cholesky cholesky.cpp $(LIBFLAGS)

omp: 
	$(CXX) $(CFLAGS) -fopenmp -o cholesky_omp cholesky_omp.cpp $(LIBFLAGS)
.PHONY:clean
clean:
	@-rm -f cholesky_omp cholesky_dag cholesky *jpg *png *csv loop_dep* 
