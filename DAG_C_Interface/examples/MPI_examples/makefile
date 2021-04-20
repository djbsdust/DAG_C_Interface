CC=mpicxx
#CFLAGS = -std=c++0x -fPIC -Wall -D_EXEC_TIME -D_RETURN_VERTICAL #-DDEBUG_GRAPH 

SCHEDULER_INSTALL_PATH=/home/lchen/dag_task_scheduler_newmerge
 
CFLAGS =  -g   -std=c++0x -Wall -I${SCHEDULER_INSTALL_PATH}/include -L${SCHEDULER_INSTALL_PATH}/lib -lpapi -L/opt/papi/lib64/

LIBFLAGS = -ltbb -ldag_task_scheduler

all: cpi 2level_3d 1level 1level_3d

cpi:
	$(CC) $(CFLAGS) -o cpi cpi_aceMesh.c $(LIBFLAGS)

clean:
	@-rm -f *.o
	@-rm -f cpi
