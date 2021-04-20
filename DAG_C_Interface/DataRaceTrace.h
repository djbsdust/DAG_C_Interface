#ifndef DATARACETRACE_H
#define DATARACETRACE_H

#include "aceMesh_runtime.h"

#ifdef DATA_RACE_TRACE
#include <unordered_map>
#include <map>
#include <set>

#define MAX(x,y) (x>y?x:y)
class aceMesh_task;

struct DataObjState {
  DataObjState(): latestReadTaskId(0), latestWriteTaskId(0) {}
  volatile int latestReadTaskId, latestWriteTaskId;
};

typedef long TaskIdT;

//bellow for store source code information
//typedef std::unordered_map<std::set<TaskIdT>*, char*> TaskId2SourceT;
//typedef std::unordered_map<void*, char*> Ptr2SourceT;
typedef std::map<std::set<TaskIdT>*, char*> TaskId2SourceT;
typedef std::map<void*, char*> Ptr2SourceT;

//bellow for store data object state: latest read task id and latest 
//write task id
//typedef std::unordered_map<void*, DataObjState> DataObjStateMapT;
//typedef std::unordered_map<void*, DataObjState>::iterator DataObjStateIteratorT;
typedef std::map<void*, DataObjState> DataObjStateMapT;
typedef std::map<void*, DataObjState>::iterator DataObjStateIteratorT;

//typedef std::unordered_map<void*, TaskIdT> TaskPtr2TaskIdT;
typedef std::map<void*, TaskIdT> TaskPtr2TaskIdT;

#define PAD_SIZE(x) ((x+sizeof(long)-1)/sizeof(long)*sizeof(long))

#define MAX_PER_BUF_LEN  16*1024*1024


Error_Code begin_split_task_wrap(char* fileName, int line);
Error_Code end_split_task_wrap(char* fileName, int line);
void spawn_and_wait_wrap();

void __EnterTaskExec(aceMesh_task* task);
void __ExitTaskExec(aceMesh_task* task);

//nonconst interface
void __ReadTrace(double* dataPtr, char* alias);
void __WriteTrace(double* dataPtr, char* alias);
void __ReadHaloTrace(double* dataPtr, char* alias);
void __WriteHaloTrace(double* dataPtr, char* alias);
void* __AddData2Trace(char* name, double* dataPtr);

//const interface
void __ReadTrace(const double* dataPtr, char* alias);
void __WriteTrace(const double* dataPtr, char* alias);
void __ReadHaloTrace(const double* dataPtr, char* alias);
void __WriteHaloTrace(const double* dataPtr, char* alias);
void* __AddData2Trace(char* name, const double* dataPtr);
aceMesh_task* __AddTask2Trace(aceMesh_task* task);

#endif

#endif

