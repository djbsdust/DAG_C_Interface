#ifndef WRAPREP_H
#define WRAPREP_H


#ifdef DATA_RACE_TRACE

#define AddData2Trace(x) __AddData2Trace(#x,x)
#define AddTask2Trace(x) __AddTask2Trace(x)

#define EnterTaskExec   __EnterTaskExec(this)
//currently, we only use EnterTaskExec
#define ExitTaskExec    __ExitTaskExec(this)

#define ReadTrace(x)    __ReadTrace(x, #x)
#define WriteTrace(x)   __WriteTrace(x, #x)
#define ReadHaloTrace(x)  __ReadHaloTrace(x, #x)
#define WriteHaloTrace(x) __WriteHaloTrace(x, #x)

#define begin_split_task()      begin_split_task_wrap(__FILE__, __LINE__)
#define end_split_task()        end_split_task_wrap(__FILE__, __LINE__)
#define spawn_and_wait          spawn_and_wait_wrap


#else

#define AddData2Trace(x)  (x)
#define AddTask2Trace(x)  (x)

#define EnterTaskExec
#define ExitTaskExec

#define ReadTrace(x)  
#define WriteTrace(x)
#define ReadHaloTrace(x)
#define WriteHaloTrace(x)

#endif
#endif

