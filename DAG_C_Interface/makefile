include make.defs.local

CC=gcc #icpc#mpiicxx#icpc
FC=gfortran #ifort#mpiifort#ifort
#CFLAGS = -std=c++0x -fPIC -Wall -D_EXEC_TIME -D_RETURN_VERTICAL   
CFLAGS = -std=c++0x -fpermissive -Wall -D_EXEC_TIME -D_RETURN_VERTICAL -I${HWLOC_HOME}/include -DNUMA_SUPPORT -DNODENUM=2 -DCORENUM=14 \
  -DNUMA_PROFILING   
#-I/opt/intel/composer_xe_2013.0.079/tbb/include/ \
  -L/opt/intel/composer_xe_2013.0.079/tbb/lib/intel64/  -ltbb
#-I/vol6/home/zhangh1/tbb44_20160413oss/include/  
LIBFLAGS = -L${HWLOC_HOME}/lib -lhwloc

ifeq ($(DEBUG),true)
CFLAGS+= -g -O0
else
CFLAGS+= -O3
endif

ifeq ($(MIC),true)
CFLAGS+= -mmic 
endif

ifeq ($(SW),true)
CFLAGS+= -DSWF 
endif

#define in head file
#the many flag for function
HEAD_DEFINE = 

ifeq ($(SCHEDULER), AceMesh_scheduler)
CFLAGS+= -DACEMESH_SCHEDULER
HEAD_DEFINE += \#define ACEMESH_SCHEDULER\n
else
	ifeq ($(SCHEDULER), dynamic_scheduler)
	CFLAGS+= -DDYNAMIC_SCHEDULER
	HEAD_DEFINE += \#define DYNAMIC_SCHEDULER\n
	endif
endif

ifeq ($(CPU_BIND), true)
ifeq ($(MIC),true)
CFLAGS+= -DCPUBIND -DMIC  #try a specific binding strategy
else
CFLAGS+= -DCPUBIND 
endif
HEAD_DEFINE += \#define CPUBIND\n
endif

#the Macro for PARTITION task_graph
ifeq ($(SUPPORT_PARTITION),AUTO)
CFLAGS+= -DAUTO_PARTITION
HEAD_DEFINE += \#define AUTO_PARTITION\n
else 
	ifeq ($(SUPPORT_PARTITION),MANUAL)
	CFLAGS+= -DSUPPORT_PARTITION
	HEAD_DEFINE += \#define SUPPORT_PARTITION\n
	endif
endif

ifeq ($(AUTO_AFFINITY),true)
CFLAGS+= -DAUTO_AFFINITY
endif

ifeq ($(ALL_CYCILC),true)
CFLAGS+= -DALL_CYCLIC
endif

ifeq ($(SPECIFY_END_TASKS),true)
  CFLAGS+= -DSPECIFY_END_TASKS
  HEAD_DEFINE += \#define SPECIFY_END_TASKS\n
endif

ifeq ($(MEMORY_POOL),true)
CFLAGS += -DMEMORY_POOL
HEAD_DEFINE += \#define MEMORY_POOL\n
endif

ifeq ($(MPI_TEST_LIGHT),true)
    CFLAGS+= -DMTEST_LIGHT -I${MY_MPI_HOME}/include
    LIBFLAGS+= -L${MY_MPI_HOME}/lib -lmpi
    HEAD_DEFINE += \#define MTEST_LIGHT\n
endif

ifeq ($(ACEMESH_PROFILING),true)
CFLAGS+= -DACEMESH_SCHEDULER_PROFILING -DACEMESH_TIME
HEAD_DEFINE += \#define ACEMESH_SCHEDULER_PROFILING\n\#define ACEMESH_TIME\n
ifeq ($(ACEMESH_PROFILING_EACH_TASK),true)
CFLAGS+= -DACEMESH_SCHEDULER_PROFILING_EACH_TASK
HEAD_DEFINE += \#define ACEMESH_SCHEDULER_PROFILING_EACH_TASK\n
endif
endif

ifeq ($(PAPI_PERFORMANCE),true)
CFLAGS+= -DACEMESH_PERFORMANCE -I$(PAPI_DIR)/include/ 
HEAD_DEFINE += \#define ACEMESH_PERFORMANCE\n
ifeq ($(PAPI_PERFORMANCE_TOTAL),true)
CFLAGS+= -DACEMESH_PAPI_PERFORMANCE_TOTAL -I$(PAPI_DIR)/include/ 
HEAD_DEFINE += \#define ACEMESH_PAPI_PERFORMANCE_TOTAL\n
else
CFLAGS+= -DACEMESH_PAPI_PERFORMANCE_EACH_TASK -I$(PAPI_DIR)/include/ 
HEAD_DEFINE += \#define ACEMESH_PAPI_PERFORMANCE_EACH_TASK\n
endif
endif


ifeq ($(PARALLEL_LEVEL),NO_PARALLEL)
CFLAGS+= -DNO_PARALLEL
HEAD_DEFINE += \#define NO_PARALLEL\n 
else
CFLAGS+= -DFOR_PARALLEL
HEAD_DEFINE += \#define FOR_PARALLEL\n 
endif

ifeq ($(DATA_RACE_TRACE),true)
CFLAGS+= -DDATA_RACE_TRACE
HEAD_DEFINE += \#define DATA_RACE_TRACE\n 
endif

ifeq ($(Profiling),true)
CFLAGS+= -DTBB_DEBUG -DTBB_USE_THREADING_TOOLS
endif

ifeq ($(Profiling),true)
LIBFLAGS+= -ltbb_debug
endif

ifeq ($(DEBUG_GRAPH),true)
CFLAGS+= -DDEBUG_GRAPH -D_EXEC_TIME -DMERGE_SAME_ADDR -DSAVE_RW_INFO #-DACEMESH_SCHEDULER -DACEMESH_TIME 
LIBFLAGS+= -ltbb_debug 
HEAD_DEFINE += \#define DEBUG_GRAPH\n \#define SAVE_RW_INFO\n 
endif

ifeq ($(CONCURRENT_CONSTRUCT_GRAPH),true)
CFLAGS+= -DCONCURRENT_CONSTRUCT_GRAPH
endif

#MPI strategy related
ifeq ($(NOT_MPI_STRATEGY),true)
  CFLAGS+= -DNOT_MPI_STRATEGY
  HEAD_DEFINE += \#define NOT_MPI_STRATEGY\n
else
  ifeq ($(EXTRA_THREAD),true)
    CFLAGS+= -DEXTRA_THREAD
    HEAD_DEFINE += \#define EXTRA_THREAD\n
    CFLAGS+= -DUSE_TBB_QUEUE_MPI
    HEAD_DEFINE += \#define USE_TBB_QUEUE_MPI\n
  else
    ifeq ($(BLOCKING_QUEUE),true)
      CFLAGS+= -DBLOCKING_QUEUE
      HEAD_DEFINE += \#define BLOCKING_QUEUE\n
    endif
  endif
endif

#Integrated with the original interface. No seperate interface required.
ifeq ($(ACEMESH_THREAD_GROUP),true)
CFLAGS += -D__ACEMESH_THREAD_GROUP
HEAD_DEFINE += \#define __ACEMESH_THREAD_GROUP\n
endif


source_h                     = aceMesh_runtime.h
source_aceMesh_cpp           = aceMesh_runtime.cpp 

source_aceMesh_task_cpp      = aceMesh_task.cpp 
source_aceMesh_task_h        = aceMesh_task.h 
source_task_dag_graph_cpp    = task_dag_graph.cpp 
source_task_dag_graph_h      = task_dag_graph.h 

source_concurrent_aceMesh_task_cpp      = concurrent_aceMesh_task.cpp 
source_concurrent_aceMesh_task_h        = concurrent_aceMesh_task.h 
source_concurrent_task_dag_graph_cpp    = concurrent_task_dag_graph.cpp 
source_concurrent_task_dag_graph_h      = concurrent_task_dag_graph.h 

source_addr_area_manager_h   = addr_area_manager.h 
source_addr_area_manager_cpp = addr_area_manager.cpp 
source_dag_graph_check_h     = dag_graph_check.h 
source_dag_graph_check_cpp   = dag_graph_check.cpp 

OBJECTS = aceMesh_runtime.o addr_area_manager.o dag_graph_check.o trace_out.o DataRaceTrace.o aceMesh_scheduler_init.o aceMesh_stack.o affinity_id_generator.o disjoin_set.o aceMesh_utils.o aceMesh_clang.o thread_group.o splitter.o tbb_thread_local.o dag_struct.mod

ifeq ($(SCHEDULER), AceMesh_scheduler)
OBJECTS += task.o scheduler.o aceMesh_deque.o aceMesh_stack.o 
endif

ifeq ($(PAPI_PERFORMANCE),true)
OBJECTS += aceMesh_performance.o
endif

ifeq ($(MEMORY_POOL),true)
OBJECTS += MemPool.o
endif

ifeq ($(PARALLEL_LEVEL),NO_PARALLEL)
OBJECTS += aceMesh_task.o task_dag_graph.o 
else
OBJECTS += concurrent_aceMesh_task.o concurrent_task_dag_graph.o  
endif

DagTaskScheduler_static:  $(OBJECTS) 
	ar r libdag_task_scheduler.a $(OBJECTS) 
	@sed "3i $(HEAD_DEFINE)" aceMesh_runtime.h > aceMesh_runtime_install.h
	@sed "3i $(HEAD_DEFINE)" aceMesh_task.h > aceMesh_task_install.h
	@sed "3i $(HEAD_DEFINE)" aceMesh_hierarchy_task.h > aceMesh_hierarchy_task_install.h
	@sed "3i $(HEAD_DEFINE)" tbb_thread_local.h > tbb_thread_local_install.h
	@sed "3i $(HEAD_DEFINE)" aceMesh_concurrent_task.h > aceMesh_concurrent_task_install.h

DagTaskScheduler_dynamic: $(OBJECTS) 
	$(CC) -fPIC -shared -o libdag_task_scheduler_static.so $(OBJECTS) 

task.o: task.cpp task.h 
	$(CC) $(CFLAGS) -c task.cpp 

MemPool.o: MemPool.c MemPool.h
	$(CC) $(CFLAGS) -c  MemPool.c

aceMesh_utils.o: aceMesh_utils.h aceMesh_utils.cpp
	$(CC) $(CFLAGS) -c aceMesh_utils.cpp

aceMesh_deque.o: aceMesh_deque.h aceMesh_deque.cpp
	$(CC) $(CFLAGS) -c aceMesh_deque.cpp 

affinity_id_generator.o: affinity_id_generator.h affinity_id_generator.cpp
	$(CC) $(CFLAGS) -c affinity_id_generator.cpp 

disjoin_set.o: disjoin_set.h disjoin_set.cpp
	$(CC) $(CFLAGS) -c disjoin_set.cpp 

aceMesh_performance.o: aceMesh_performance.cpp aceMesh_performance.h
	$(CC) $(CFLAGS) -c aceMesh_performance.cpp

aceMesh_thread.o: aceMesh_thread.cpp aceMesh_thread.h 
	$(CC) $(CFLAGS) -c aceMesh_thread.cpp 

aceMesh_stack.o: aceMesh_stack.cpp aceMesh_stack.h 
	$(CC) $(CFLAGS) -c aceMesh_stack.cpp 

scheduler.o: scheduler.cpp scheduler.h 
	$(CC) $(CFLAGS) -c scheduler.cpp 

aceMesh_task.o: $(source_aceMesh_task_h) 
	$(CC) $(CFLAGS) -c $(source_aceMesh_task_cpp) 

concurrent_aceMesh_task.o: $(source_concurrent_aceMesh_task_h) 
	$(CC) $(CFLAGS) -c $(source_concurrent_aceMesh_task_cpp) 

aceMesh_runtime.o: $(source_aceMesh_cpp) $(source_h) aceMesh_performance.h 
	$(CC) $(CFLAGS) -c $(source_aceMesh_cpp)  

task_dag_graph.o: $(source_task_dag_graph_cpp) $(source_h)  $(source_task_dag_graph_h)
	$(CC) $(CFLAGS) -c $(source_task_dag_graph_cpp) 

concurrent_task_dag_graph.o: $(source_concurrent_task_dag_graph_cpp) $(source_h)  $(source_concurrent_task_dag_graph_h)
	$(CC) $(CFLAGS) -c $(source_concurrent_task_dag_graph_cpp)  

addr_area_manager.o: $(source_addr_area_manager_h) $(source_addr_area_manager_cpp)
	$(CC) $(CFLAGS) -c $(source_addr_area_manager_cpp) 

dag_graph_check.o: $(source_dag_graph_check_h) $(source_h) 
	$(CC) $(CFLAGS) -c  $(source_dag_graph_check_cpp)

trace_out.o: trace_out.cpp aceMesh_runtime.h
	$(CC) $(CFLAGS) -c trace_out.cpp

ifeq ($(SCHEDULER), AceMesh_scheduler)
aceMesh_scheduler_init.o: aceMesh_scheduler_init_v2.cpp aceMesh_scheduler_init_v2.h scheduler.h
	$(CC) $(CFLAGS) -c aceMesh_scheduler_init_v2.cpp -o aceMesh_scheduler_init.o
else

ifeq ($(SCHEDULER), dynamic_scheduler)
aceMesh_scheduler_init.o: aceMesh_scheduler_init.cpp aceMesh_scheduler_init.h
	$(CC) $(CFLAGS) -c aceMesh_scheduler_init.cpp
endif

endif

DataRaceTrace.o: DataRaceTrace.cpp DataRaceTrace.h
	$(CC) $(CFLAGS) -c DataRaceTrace.cpp

thread_group.o: thread_group.h thread_group.cpp
	$(CC) $(CFLAGS) -c thread_group.cpp

splitter.o: splitter.cpp splitter.h
	$(CC) $(CFLAGS) -c splitter.cpp

tbb_thread_local.o: tbb_thread_local.cpp tbb_thread_local.h
	$(CC) $(CFLAGS) -c tbb_thread_local.cpp

aceMesh_clang.o:   
	$(CC) $(CFLAGS) -c aceMesh_clang.cpp
dag_struct.mod: 
	$(FC) -c fmodule.f90
clean:
	-rm -f *.o  *mod > /dev/null 2>&1
	-rm -f *.a > /dev/null 2>&1
	-rm -f *.so > /dev/null 2>&1
	-rm -f *_install.h > /dev/null 2>&1
