#include "aceMesh_thread.h"


/*
aceMesh_thread::aceMesh_thread( __ACEMESH_NATIVE_THREAD_ROUTINE_PTR(start_routine), void* closure)
{
    pthread_t thread_handle;
    int status;
    pthread_attr_t stack_size;
    status = pthread_attr_init( &stack_size );
    if( status )
        handle_perror( status, "pthread_attr_init" );
    status = pthread_attr_setstacksize( &stack_size, ThreadStackSize );
    if( status )
        handle_perror( status, "pthread_attr_setstacksize" );

    status = pthread_create( &thread_handle, &stack_size, start_routine, closure );
    if( status )
        handle_perror( status, "pthread_create" );

    my_handle = thread_handle;
}*/

/*
aceMesh_thread::~aceMesh_thread()
{

}*/

/*
void aceMesh_thread::internal_start( __ACEMESH_NATIVE_THREAD_ROUTINE_PTR(start_routine), void* args)
{
    pthread_t thread_handle;
    int status;
    pthread_attr_t stack_size;
    status = pthread_attr_init( &stack_size );
    if( status )
        handle_perror( status, "pthread_attr_init" );
    status = pthread_attr_setstacksize( &stack_size, ThreadStackSize );
    if( status )
        handle_perror( status, "pthread_attr_setstacksize" );

    status = pthread_create( &thread_handle, &stack_size, start_routine, closure );
    if( status )
        handle_perror( status, "pthread_create" );

    my_handle = thread_handle;
}*/
