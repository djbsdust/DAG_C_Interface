
namespace AceMesh_runtime {

#define STATIC_STACK_STRATEGY
//#define STATIC_QUEUE_STRATEGY
//#define STATIC_PRIORITY_QUEUE_STRATEGY

//#define STATIC_DEQUE_STRATEGY
//#define STATIC_DYNAMIC_STRATEGY
//#define DYNAMIC_PRIORITY_QUEUE_STRATEGY
//#define STATIC_DYNAMIC_STRATEGY1
//#define STATIC_DYNAMIC_STRATEGY2
//#define STATIC_DYNAMIC_STRATEGY3
//#define STATIC_DYNAMIC_STRATEGY4

//no stealing
#ifdef STATIC_STACK_STRATEGY
    //#for private queue
    #define USE_STACK
    //for shared queue
//    #define USE_SHARED_STACK
#endif

#ifdef STATIC_QUEUE_STRATEGY
    //#for private queue
    #define USE_TBB_QUEUE
    //for shared queue
//    #define USE_SHARED_STACK
#endif

#ifdef STATIC_PRIORITY_QUEUE_STRATEGY
    //#for private queue
    #define USE_PRIORITY_QUEUE
    //for shared queue
//    #define USE_SHARED_STACK
#endif

#ifdef STATIC_DEQUE_STRATEGY
    //#for private queue
    #define USE_DEQUE
    //for shared queue
//    #define USE_SHARED_STACK
    #define USE_MY_OPT_STRATEGY 
#endif

//have stealing
//this deque is implemented incorrectly! it may incur livelock
#ifdef STATIC_DYNAMIC_STRATEGY
    //#for private queue
    #define USE_DEQUE
    //for shared queue
//    #define USE_SHARED_STACK
    #define USE_STEALING 
    #define USE_MY_OPT_STRATEGY 
#endif

#ifdef STATIC_DYNAMIC_STRATEGY1
    //#for private queue
    #define USE_STACK
    //for shared queue
//    #define USE_SHARED_STACK
    #define USE_STEALING 
#endif

#ifdef STATIC_DYNAMIC_STRATEGY2
    //#for private queue
    #define USE_TBB_QUEUE
    //for shared queue
//    #define USE_SHARED_STACK
    #define USE_STEALING 
#endif

#ifdef STATIC_DYNAMIC_STRATEGY3
    //#for private queue
    #define USE_STACK
    //for shared queue
//    #define USE_SHARED_STACK
    #define USE_STEALING_NUMA
#endif
#ifdef STATIC_DYNAMIC_STRATEGY4
    //#for private queue
    #define USE_STACK
    //for shared queue
//    #define USE_SHARED_STACK
    #define USE_STEALING_SHARED
#endif


#ifdef DYNAMIC_PRIORITY_QUEUE_STRATEGY
    #define USE_PRIORITY_QUEUE
//    #define USE_SHARED_STACK
    #define USE_STEALING 
#endif



//#for private queue
//#define USE_TBB_QUEUE
//#define USE_STACK
//#define USE_P_QUEUE
//define USE_DEQUE

//for shared queue
//#define USE_SHARED_STACK
//#define USE_SHARED_QUEUE 

//for scheduler, only support DEQUE
//#define USE_STEALING 
//#define USE_MY_OPT_STRATEGY 


}
