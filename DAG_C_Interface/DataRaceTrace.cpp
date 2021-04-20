#include "DataRaceTrace.h"
#include <cassert>
#include <cstring>
#include <pthread.h>

#ifdef DATA_RACE_TRACE

DataObjStateMapT DataObjStateMap;
TaskId2SourceT Id2LoopLocation;
Ptr2SourceT DataPtr2DataName;
TaskPtr2TaskIdT TaskPtr2Id;

//static buffer to avoid malloc and free, 
//period means buffer should be recyled now and then
char PeriodRecycleBuf[MAX_PER_BUF_LEN];
int CurrentBufPos=0;
char PeriodRecycleVarNameBuf[MAX_PER_BUF_LEN];
int CurrentVNameBufPos=0;
std::set<TaskIdT> PeriodRecycleTaskSetBuf[MAX_PER_BUF_LEN];
int CurrentTaskSetBufPos=0;

std::set<TaskIdT>* CurrentTaskSet;
char* CurrentLoopLocationDescript;

TaskIdT GlobalTaskId=0;

pthread_key_t LocalTaskKey;
bool LocalTaskKeyIsInitialized = false;

Error_Code begin_split_task_wrap(char* fileName, int line) {
  CurrentTaskSet = &PeriodRecycleTaskSetBuf[CurrentTaskSetBufPos++];
  CurrentTaskSet->clear();
  CurrentLoopLocationDescript = &PeriodRecycleBuf[CurrentBufPos];

  int len = strlen(fileName);
  for (int i=0; i<len; i++)
    PeriodRecycleBuf[CurrentBufPos++] = fileName[i];
  PeriodRecycleBuf[CurrentBufPos++] = ':';
  char tmpBuf[100];
  sprintf(tmpBuf, "%d", line);
  len = strlen(tmpBuf);
  strcpy(&PeriodRecycleBuf[CurrentBufPos], tmpBuf);
  CurrentBufPos += len;
  sprintf(tmpBuf, " TaskId:%ld-", GlobalTaskId);
  strcpy(&PeriodRecycleBuf[CurrentBufPos], tmpBuf);
  CurrentBufPos += strlen(tmpBuf);

  if (CurrentBufPos > MAX_PER_BUF_LEN) {
    printf("PeriodRecycleBuf overfloor!\n");
    assert(0);
  }

  if (!LocalTaskKeyIsInitialized) {
    pthread_key_create(&LocalTaskKey, NULL);
    LocalTaskKeyIsInitialized = true;
  }

  return begin_split_task();
}

Error_Code end_split_task_wrap(char* fileName, int line) {
  char tmpBuf[100];
  sprintf(tmpBuf, "%ld", GlobalTaskId);
  strcpy(&PeriodRecycleBuf[CurrentBufPos], tmpBuf);
  CurrentBufPos += strlen(tmpBuf);
  CurrentBufPos++; //avoid rewrite '\0'

  CurrentBufPos = PAD_SIZE(CurrentBufPos);
  if (CurrentBufPos > MAX_PER_BUF_LEN) {
    printf("PeriodRecycleBuf overfloor!\n");
    assert(0);
  }

  Id2LoopLocation.insert(std::pair<std::set<TaskIdT>*, char*>
      (CurrentTaskSet, CurrentLoopLocationDescript));
  CurrentTaskSet=NULL;
  
  return end_split_task();
}

void spawn_and_wait_wrap() {
  spawn_and_wait();

  DataObjStateMap.clear();
  DataPtr2DataName.clear();
  Id2LoopLocation.clear();
  TaskPtr2Id.clear();

  CurrentVNameBufPos=0;
  CurrentBufPos=0;
  CurrentTaskSetBufPos=0;
}

void __ExitTaskExec(aceMesh_task* task) {
  //TODO: not implemented
}

void __EnterTaskExec(aceMesh_task* task) {
  pthread_setspecific(LocalTaskKey, task);
}

char* FindLoopLocationByTaskId(TaskIdT taskId) {
  for (int i=0; i<CurrentTaskSetBufPos; i++) {
    if (PeriodRecycleTaskSetBuf[i].find(taskId) != 
        PeriodRecycleTaskSetBuf[i].end()) {
      return Id2LoopLocation.find(&PeriodRecycleTaskSetBuf[i])->second;
    }
  }
  return NULL;
}

void WriteAfterWriteError(TaskIdT writeTaskIdNew, char* aliasNew, 
    TaskIdT writeTaskIdOld, char* aliasOld) {
  char* writeLoopLocationNew = FindLoopLocationByTaskId(writeTaskIdNew);
  char* writeLoopLocationOld = FindLoopLocationByTaskId(writeTaskIdOld);
  char buf[1024], tmp[1024];
  strcpy(buf,"Error! Write After Write Dependence Broken! ");
  if (aliasOld != NULL) {
    sprintf(tmp, "Old Write at %s variable %s,", writeLoopLocationOld, aliasOld);
  } else {
    sprintf(tmp, "Old Write at %s,", writeLoopLocationOld);
  }
  strcat(buf, tmp);
  if (aliasNew != NULL) {
    sprintf(tmp, "New Write at %s variable %s\n", writeLoopLocationNew, aliasNew);
  } else {
    sprintf(tmp, "New Write at %s\n", writeLoopLocationOld);
  } 
  strcat(buf, tmp);

  printf("%s", buf);
  assert(0);
}


void ReadAfterWriteError(TaskIdT readTaskId, char* aliasRead, 
    TaskIdT writeTaskId, char* aliasWrite) {
  char* readLoopLocation = FindLoopLocationByTaskId(readTaskId);
  char* writeLoopLocation = FindLoopLocationByTaskId(writeTaskId);
  char buf[1024], tmp[1024];
  strcpy(buf, "Error! Read After Write Dependence Broken! ");
  if (aliasRead != NULL) {
    sprintf(tmp, "Read at %s variable %s,", readLoopLocation, aliasRead);
  } else {
    sprintf(tmp, "Read at %s,", readLoopLocation);
  }
  strcat(buf, tmp);
  if (aliasWrite != NULL) {
    sprintf(tmp, "Write at %s variable %s\n", writeLoopLocation, aliasWrite);
  } else {
    sprintf(tmp, "Write at %s\n", writeLoopLocation);
  }
  strcat(buf, tmp);

  printf("%s", buf);
  assert(0);
}

void __ReadHaloTrace(double* dataPtr, char* alias) {
  aceMesh_task* task = (aceMesh_task*)pthread_getspecific(LocalTaskKey);
  if (task == NULL) {
    printf("Fatal error! ReadHaloTrace can't find current task pointer in TLS\n");
    assert(0);
  }
  if (TaskPtr2Id.find(task) == TaskPtr2Id.end()) {
    printf("Fatal error! can't find task in map TaskPtr2Id\n");
    assert(0);
  }
  if (DataObjStateMap.find(dataPtr) != DataObjStateMap.end()) {
    DataObjState& state = DataObjStateMap.find(dataPtr)->second;
    TaskIdT taskId = TaskPtr2Id.find(task)->second;
    state.latestReadTaskId = MAX(taskId, state.latestReadTaskId);

  } else {
    TaskIdT taskId = TaskPtr2Id.find(task)->second;
    char* loopDesc = FindLoopLocationByTaskId(taskId);
    printf("Fatal error! can't find DataObjState of %s in DataObjStateMap. "
        "Task loc:%s \n",
        alias, loopDesc);
    assert(0);
  }
}

void __ReadTrace(double* dataPtr, char* alias) {
  aceMesh_task* task = (aceMesh_task*)pthread_getspecific(LocalTaskKey);
  if (task == NULL) {
    printf("Fatal error! ReadTrace can't find current task pointer in TLS\n");
    assert(0);
  }
  if (TaskPtr2Id.find(task) == TaskPtr2Id.end()) {
    printf("Fatal error! can't find task in map TaskPtr2Id\n");
    assert(0);
  }
  if (DataObjStateMap.find(dataPtr) != DataObjStateMap.end()) {
    DataObjState& state = DataObjStateMap.find(dataPtr)->second;
    TaskIdT taskId = TaskPtr2Id.find(task)->second;
    if (state.latestWriteTaskId > taskId) {
      ReadAfterWriteError(taskId, alias, state.latestWriteTaskId, NULL);
    }
    state.latestReadTaskId = MAX(taskId, state.latestReadTaskId);

  } else {
    TaskIdT taskId = TaskPtr2Id.find(task)->second;
    char* loopDesc = FindLoopLocationByTaskId(taskId);
    printf("Fatal error! can't find DataObjState of %s in DataObjStateMap. "
        "Task loc:%s \n",
        alias, loopDesc);
    assert(0);
  }
}

void __WriteHaloTrace(double* dataPtr, char* alias) {
  aceMesh_task* task = (aceMesh_task*)pthread_getspecific(LocalTaskKey);
  if (task == NULL) {
    printf("Fatal error! WriteHaloTrace can't find current task pointer in TLS\n");
    assert(0);
  }
  if (TaskPtr2Id.find(task) == TaskPtr2Id.end()) {
    printf("Fatal error! can't find task in map TaskPtr2Id\n");
    assert(0);
  }
  if (DataObjStateMap.find(dataPtr) != DataObjStateMap.end()) {
    DataObjState& state = DataObjStateMap.find(dataPtr)->second;
    TaskIdT taskId = TaskPtr2Id.find(task)->second;
    state.latestWriteTaskId = MAX(taskId, state.latestWriteTaskId);

  } else {
    TaskIdT taskId = TaskPtr2Id.find(task)->second;
    char* loopDesc = FindLoopLocationByTaskId(taskId);
    printf("Fatal error! can't find DataObjState of %s in DataObjStateMap. Task loc:%s \n",
        alias, loopDesc);
    assert(0);
  }
}


void __WriteTrace(double* dataPtr, char* alias) {
  aceMesh_task* task = (aceMesh_task*)pthread_getspecific(LocalTaskKey);
  if (task == NULL) {
    printf("Fatal error! WriteTrace can't find current task pointer in TLS\n");
    assert(0);
  }
  if (TaskPtr2Id.find(task) == TaskPtr2Id.end()) {
    printf("Fatal error! can't find task in map TaskPtr2Id\n");
    assert(0);
  }
  if (DataObjStateMap.find(dataPtr) != DataObjStateMap.end()) {
    DataObjState& state = DataObjStateMap.find(dataPtr)->second;
    TaskIdT taskId = TaskPtr2Id.find(task)->second;
    if (state.latestReadTaskId > taskId) {
      ReadAfterWriteError(state.latestReadTaskId, NULL, taskId, alias);
    }
    if (state.latestWriteTaskId > taskId) {
      WriteAfterWriteError(state.latestWriteTaskId, NULL, taskId, alias);
    }
    state.latestWriteTaskId = MAX(taskId, state.latestWriteTaskId);

  } else {
    TaskIdT taskId = TaskPtr2Id.find(task)->second;
    char* loopDesc = FindLoopLocationByTaskId(taskId);
    printf("Fatal error! can't find DataObjState of %s in DataObjStateMap. Task loc:%s \n",
        alias, loopDesc);
    assert(0);
  }
}

void* __AddData2Trace(char* name, double* dataPtr) {
  if (DataPtr2DataName.find(dataPtr) == DataPtr2DataName.end()) {
    char* dataName = &PeriodRecycleVarNameBuf[CurrentVNameBufPos];
    strcpy(&PeriodRecycleVarNameBuf[CurrentVNameBufPos], name);
    CurrentVNameBufPos += strlen(name);
    CurrentVNameBufPos++;
    CurrentVNameBufPos = PAD_SIZE(CurrentVNameBufPos);

    DataPtr2DataName.insert(std::pair<void*, char*>  
        (dataPtr, dataName));
    DataObjStateMap.insert(std::pair<void*, DataObjState>
        (dataPtr, DataObjState()));
  }

  return dataPtr;
}

void __ReadTrace(const double* dataPtr, char* alias) {
  __ReadTrace(const_cast<double*>(dataPtr), alias);
}
void __WriteTrace(const double* dataPtr, char* alias) {
  __WriteTrace(const_cast<double*>(dataPtr), alias);
}
void __ReadHaloTrace(const double* dataPtr, char* alias) {
  __ReadHaloTrace(const_cast<double*>(dataPtr), alias);
}
void __WriteHaloTrace(const double* dataPtr, char* alias) {
  __WriteHaloTrace(const_cast<double*>(dataPtr), alias);
}
void* __AddData2Trace(char* name, const double* dataPtr) {
  return __AddData2Trace(name, const_cast<double*>(dataPtr));
}

aceMesh_task* __AddTask2Trace(aceMesh_task* task) {
  if (TaskPtr2Id.find(task) == TaskPtr2Id.end()) {
    int currentTaskId = GlobalTaskId++; 
    //here function should be involved serial!
    TaskPtr2Id.insert(std::pair<void*, TaskIdT>
        (task, currentTaskId));
    CurrentTaskSet->insert(currentTaskId);
  }
  return task;
}
#endif

