UPCC=upcc
UPC_FLAGS= -T=1  --uses-threads

MIC = 
SCHEDULER_INSTALL_PATH=${DAG_HOME}
ifeq ($(MIC),TRUE)
EXTRA_FLAGS= -Dnx=4096 -Dny=4096 
else
EXTRA_FLAGS= -Dnx=4096 -Dny=4096 
endif
DEBUG= -DCHECK
OPT= -O3

# for dag
CFLAGS1=  $(OPT) $(DEBUG) -mcmodel=medium  -Wall -I${SCHEDULER_INSTALL_PATH}/include -L${SCHEDULER_INSTALL_PATH}/lib 
LIBFLAGS = -L${SCHEDULER_INSTALL_PATH}/lib -ldag_task_scheduler  -ltbb

#for omp and serial
CFLAGS2=  $(OPT) $(DEBUG) 

ifeq ($(MIC),TRUE)
CXX=icc -mmic
CC=icc -mmic
LIBFLAGS= -L${SCHEDULER_INSTALL_PATH}/lib  -L/opt/intel/tbb42/lib/mic/ -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -ltbb -ldag_task_scheduler -lpthread -lrt# -L/opt/papi/lib64/
else
CXX=icc 
CC=icc
endif

all: dag omp
dag_1d:
	$(CC) $(CFLAGS1) $(EXTRA_FLAGS)  -o fdtd_dag  fdtd_dag.c $(LIBFLAGS)
omp_1d:
	$(CC) $(CFLAGS2) $(EXTRA_FLAGS) -fopenmp  fdtd_omp.c -o fdtd_omp
serial:
	$(CXX) $(CFLAGS2) $(EXTRA_FLAGS) fdtd_serial.cpp -o fdtd_serial

serial_bk:
	$(CC) $(CFLAGS2) $(EXTRA_FLAGS) fdtd_serial_bk.c -o fdtd_serial_bk
omp:
	$(CC) $(CFLAGS2) $(EXTRA_FLAGS) -fopenmp  fdtd_omp_bk.c -o fdtd_omp  $(LIBFLAGS)
dag:
	$(CC) $(CFLAGS1) $(EXTRA_FLAGS)  -o fdtd_dag  fdtd_dag_bk.c $(LIBFLAGS)
upch: fdtd_dag_bk.o
	$(UPCC) $(UPC_FLAGS)  -o fdtd_upch fdtd_dag_bk.o $(LIBFLAGS) -lstdc++
fdtd_dag_bk.o:  fdtd_dag_bk.c
	$(UPCC) -c $(UPC_FLAGS)  $(DEBUG)  $(EXTRA_FLAGS) -I${DAG_HOME}/include  fdtd_dag_bk.c

.PHONY:clean	
clean:
	@-rm -f fdtd_dag 
	@-rm -f fdtd_omp 
	@-rm -f fdtd_serial 
	@-rm -f fdtd_omp_bk 
	@-rm -f fdtd_dag_bk 
	@-rm -f fdtd_serial_bk *csv *png *jpg loop_dep*  
