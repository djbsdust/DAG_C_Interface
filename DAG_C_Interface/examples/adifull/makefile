
MIC=

SCHEDULER_INSTALL_PATH=${DAG_HOME}
DEBUG = -DCHECK #-g 
OPT = -O3

UPCC=upcc #-vv
UPC_FLAGS= -T=1 --uses-threads -O -opt #-save-temps

CXX = icc 
CC = icpc 
FC = ifort 
LIBFLAGS =  -L${SCHEDULER_INSTALL_PATH}/lib -ldag_task_scheduler -ltbb -lrt

ifeq ($(MIC),TRUE)
CXX = icc -mmic
CC = icpc -mmic
FC = ifort -mmic
LIBFLAGS= -L/opt/intel/tbb42/lib/mic/ -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -L/opt/mpss/3.1.2/sysroots/k1om-mpss-linux/lib64/ -L${SCHEDULER_INSTALL_PATH}/lib -ldag_task_scheduler -lpthread -ltbb  -lrt
endif

ifeq ($(MIC),TRUE)
EXTRA_FLAGS=-DN=2048 #-DT=40
UPCH_EXTRA_FLAGS=-DDIM_N=2048 #-DT=40
else
EXTRA_FLAGS=-DN=4096 #-DT=40
UPCH_EXTRA_FLAGS=-DDIM_N=4096 #-DT=40
endif
CFLAGS = $(DEBUG) $(OPT) $(EXTRA_FLAGS) -Wall -I${SCHEDULER_INSTALL_PATH}/include


all: omp  dag
omp:
	$(CC) $(EXTRA_FLAGS) $(DEBUG) $(OPT) -fopenmp  omp-adifull.c -o adifull_omp
serial: 
	$(CC) $(EXTRA_FLAGS) $(DEBUG) $(OPT)  serial-adifull.c -o adifull_serial
dag:
	$(CC) $(CFLAGS) -o adifull_dag dag-adifull.c $(LIBFLAGS)
mdag:
	$(CC) $(CFLAGS) -o adifull_mdag mdag-adifull.c $(LIBFLAGS)
dag-2d:
	$(CC) $(CFLAGS) -o adifull_dag2d dag-adifull-2d.c $(LIBFLAGS)
upch: upch-adifull.o
	$(UPCC) $(UPC_FLAGS) $(DEBUG) -o adifull_upch upch-adifull.o $(LIBFLAGS) -lstdc++ 
upch-adifull.o: upch-adifull.c
	$(UPCC) -c $(UPC_FLAGS) $(DEBUG)  $(UPCH_EXTRA_FLAGS) -I${DAG_HOME}/include -o upch-adifull.o upch-adifull.c


.PHONY:clean
clean:
	@-rm -f *o *upc adifull_*
	@-rm -f  *csv *jpg *png loop_de* 
