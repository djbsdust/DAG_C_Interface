MIC=

SCHEDULER_INSTALL_PATH=${DAG_HOME}
DEBUG = -DCHECK #-g 
OPT = -O3

UPCC=upcc #-vv
UPC_FLAGS= -T=1 --uses-threads -O -opt #-save-temps

CXX = icc 
CC = icpc 
FC = ifort 
LIBFLAGS =  -L${SCHEDULER_INSTALL_PATH}/lib -lpthread -ldag_task_scheduler -ltbb -lrt

ifeq ($(MIC),TRUE)
CXX = icc -mmic
CC = icpc -mmic
FC = ifort -mmic
LIBFLAGS= -L/opt/intel/tbb42/lib/mic/ -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -L${SCHEDULER_INSTALL_PATH}/lib -ldag_task_scheduler -lpthread -ltbb  -lrt
endif

ifeq ($(MIC),TRUE)
EXTRA_FLAGS= -DSIZEX=512 -DSIZEY=512 -DSIZEZ=128
else
EXTRA_FLAGS= -DSIZEX=512 -DSIZEY=512 -DSIZEZ=128
endif
CFLAGS = $(DEBUG) $(OPT) $(EXTRA_FLAGS) -Wall -I${SCHEDULER_INSTALL_PATH}/include

EXTRA_FFLAGS= $(UPC_FLAGS) -DBLKX=16 -DBLKY=16 -DITER=40
FFLAGS = $(DEBUG) $(OPT) $(EXTRA_FFLAGS) -cpp -free -I${SCHEDULER_INSTALL_PATH}/include 
 
all:dag omp
dag:
	$(CC) $(CFLAGS) -o 3d7p_dag 3d7p_dag.c $(LIBFLAGS)
fdag:
	$(FC) $(FFLAGS) -o 3d7p_dag 3d7p_dag.f timers.f $(LIBFLAGS) -lstdc++
upch:  3d7p_dag.o
	$(UPCC) $(UPC_FLAGS) $(DEBUG) -o 3d7p_upch 3d7p_dag.o $(LIBFLAGS) -lstdc++ 
3d7p_dag.o:  3d7p_dag.c
	$(UPCC) -c $(UPC_FLAGS) $(DEBUG)  $(EXTRA_FLAGS) -I${DAG_HOME}/include -o 3d7p_dag.o 3d7p_dag.c 
omp: 
	$(CXX) $(DEBUG) $(OPT) $(EXTRA_FLAGS) -Wall -fopenmp -o 3d7p_omp 3d7p_omp.cpp

clean:
	@-rm -f *.o
	@-rm -f 3d7p_dag 3d7p_omp *upch 
	@-rm -f *mod *csv *jpg *png loop_de* 
