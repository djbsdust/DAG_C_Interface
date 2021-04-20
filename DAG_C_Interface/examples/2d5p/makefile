
MIC=

SCHEDULER_INSTALL_PATH=${DAG_HOME}
DEBUG = -DCHECK #-g 
OPT = -O3

UPCC=upcc
UPC_FLAGS= -T=1 --uses-threads -O -opt #-save-temps

ifeq ($(MIC),TRUE)
CXX = icc -mmic
CC = icpc -mmic
FC = ifort -mmic
LIBFLAGS= -L/opt/intel/tbb42/lib/mic/ -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -L${SCHEDULER_INSTALL_PATH}/lib -ldag_task_scheduler -lpthread -ltbb  -lrt
else
CXX = icc 
CC = icpc 
FC = ifort 
LIBFLAGS =  -L${SCHEDULER_INSTALL_PATH}/lib -ldag_task_scheduler -ltbb -lrt
endif

ifeq ($(MIC),TRUE)
EXTRA_FLAGS= -DSIZEX=4096 -DSIZEY=4096
else
EXTRA_FLAGS= -DSIZEX=4096 -DSIZEY=4096
endif
CFLAGS = $(DEBUG) $(OPT) $(EXTRA_FLAGS) -Wall -I${SCHEDULER_INSTALL_PATH}/include


all: dag omp

dag:
	$(CC) $(CFLAGS) $(DEBUG) -o 2d5p_dag 2d5p_dag.c $(LIBFLAGS)
upch:  2d5p_dag.o
	$(UPCC) $(UPC_FLAGS) $(DEBUG) -o 2d5p_upch 2d5p_dag.o $(LIBFLAGS) -lstdc++ 
2d5p_dag.o:  2d5p_dag.c
	$(UPCC) -c $(UPC_FLAGS) $(DEBUG)  $(EXTRA_FLAGS) -I${DAG_HOME}/include -o 2d5p_dag.o 2d5p_dag.c 
omp: 
	$(CXX) $(DEBUG) $(OPT) $(EXTRA_FLAGS)  -fopenmp -o 2d5p_omp 2d5p_omp.cpp 

clean:
	@-rm -f *.o 
	@-rm -f 2d5p_dag 2d5p_omp *upch 
	@-rm -f *mod *csv *jpg *png loop_de* 
