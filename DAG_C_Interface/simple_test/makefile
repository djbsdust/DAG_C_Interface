CXX=icpc
LD=icpc
#CFLAGS = -std=c++0x -g -D_RETURN_VERTICAL -DCHECK #-DDEBUG_GRAPH -I../  
CFLAGS = -std=c++0x -Wall -g  -D_EXEC_TIME -D_RETURN_VERTICAL -DNO_PARALLEL -D__ACEMESH_THREAD_GROUP
#CFLAGS += -DACEMESH_SCHEDULER -DACEMESH_TIME -DDEBUG_GRAPH -D_EXEC_TIME -DMERGE_SAME_ADDR

INCLUDE = -I../
LIBFLAGS= -ltbb

test_source = test_clang_interface.cpp
exclude_src = ../concurrent_task_dag_graph.cpp ../concurrent_aceMesh_task.cpp

testobjs = $(patsubst %,%.o,$(basename $(test_source)))
exes = $(patsubst %,%.ex,$(basename $(test_source)))

all : $(exes)

srcs = $(filter-out $(exclude_src), $(wildcard ../*.cpp))
heads = $(wildcard ../*.h)
objects = $(patsubst %,%.o,$(basename $(srcs)))


%.ex : $(objects) $(testobjs)
	$(LD) $(INCLUDE) $(LIBFLAGS)  $(objects) $(testobjs) -o $@

%.o : %.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDE) $< -o $@

../%.o : ../%.cpp $(heads)
	$(CXX) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(testobjs)
	rm -f $(objects)
	rm -f $(exes)

printsth :
	echo $(srcs)
	echo $(objects)
