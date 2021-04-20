#ifdef _ACEMESH_THREAD_H
#define _ACEMESH_THREAD_H


#define __ACEMESH_NATIVE_THREAD_ROUTINE void*
#define __ACEMESH_NATIVE_THREAD_ROUTINE_PTR(r) void* (*r)( void* )

#include <pthread.h>

class aceMesh_thread
{
private:
    pthread_t my_handle;
public:
    /*
    template <class F> explicit aceMesh_thread(F f) 
    {
        typedef thread_closure_0<F> closure_type;
        internal_start(closure_type::start_routine, new closure_type(f));
    }*/
    ~aceMesh_thread();
    aceMesh_thread(__ACEMESH_NATIVE_THREAD_ROUTINE_PTR(r), void* closure);
    //void internal_start( __ACEMESH_NATIVE_THREAD_ROUTINE_PTR(start_routine), void* args);
    void internal_start( void* args );
    //bool joinable();
    //void close();
};

/*
struct aceMesh_thread_arg 
{
    int id;
    aceMesh_thread_arg():id(0){}
    aceMesh_thread_arg(int index):id(index){}
};

template<class F> struct thread_closure_0: thread_closure_base {
    F function;

    static __TBB_NATIVE_THREAD_ROUTINE start_routine( void* c ) {
        thread_closure_0 *self = static_cast<thread_closure_0*>(c);
        self->function();
        delete self;
        return 0;
    }
    thread_closure_0( const F& f ) : function(f) {}
};
//! Structure used to pass user function with 1 argument to thread.  
template<class F, class X> struct thread_closure_1: thread_closure_base {
    F function;
    X arg1;
    //! Routine passed to Windows's _beginthreadex by thread::internal_start() inside tbb.dll
    static __TBB_NATIVE_THREAD_ROUTINE start_routine( void* c ) {
        thread_closure_1 *self = static_cast<thread_closure_1*>(c);
        self->function(self->arg1);
        delete self;
        return 0;
    }
    thread_closure_1( const F& f, const X& x ) : function(f), arg1(x) {}
};
*/
#endif
