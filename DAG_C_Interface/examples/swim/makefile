
FC=ifort
OPT= -O

FFLAGS =    -cpp -free -I${DAG_HOME}/include -I./
LIBFLAGS =  -lstdc++ -L${DAG_HOME}/lib -ltbb -ldag_task_scheduler
all: fomp fdag
fdag:
	${FC} -g -c dag-swim-addon.f ${FFLAGS} 
	${FC} -g -c dag-swim.f ${FFLAGS} 
	${FC} -g -o dag-swim dag-swim.o dag-swim-addon.o ${LIBFLAGS}
fomp:
	${FC} -O -o omp-swim omp-swim.f -cpp -free -openmp

clean:
	@-rm -f *.o *csv *png *jpg  loop_*
	@-rm -f omp-swim dag-swim
	@-rm -f *.mod  
