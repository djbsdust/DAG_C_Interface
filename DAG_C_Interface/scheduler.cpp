#include "scheduler.h"
#include "aceMesh_thread.h"
#include <cassert>
#include "aceMesh_task.h"
#include "aceMesh_performance.h"
#include "aceMesh_runtime.h"
#include "aceMesh_utils.h"

#include <iostream>
#ifdef MTEST_LIGHT
#include "mpi.h"
#endif

extern int sche_num_threads;
extern int my_mpi_rank;
#ifdef MTEST_LIGHT
int *comm1 = NULL;
int *comm2 = NULL;
int mkind1; // 1:isend 2:irecv 3:icollect
int mkind2; // 1:isend 2:irecv 3:icollect
#endif
namespace AceMesh_runtime
{

#define NO_AFFINITY -1
    //int sche_num_threads = 1;

    std::vector<generic_scheduler *> generic_scheduler::schedulers;
#ifdef EXTRA_THREAD                                      //add wangm
    generic_scheduler *generic_scheduler::mpi_scheduler; //lchen
    pthread_t generic_scheduler::mpi_thread;             //lchen
#ifdef USE_TBB_QUEUE_MPI
    tbb::concurrent_queue<task *> generic_scheduler ::mpi_queue;
#else
    concurrent_aceMesh_queue generic_scheduler::mpi_queue;
#endif
#ifdef MTEST_LIGHT
    aceMesh_mtest_queue generic_scheduler::polling_queue;
#endif

#endif
    std::vector<pthread_t> generic_scheduler::threads;

    int generic_scheduler::num_threads = 1;
    tbb::atomic<int> generic_scheduler::index = tbb::atomic<int>();

    basic_tls<generic_scheduler *> generic_scheduler::theTLS;

    tbb::atomic<bool> close_all_threads = tbb::atomic<bool>();
    tbb::atomic<bool> is_run = tbb::atomic<bool>();

//tbb::atomic<bool> is_go = tbb::atomic<bool>();
#ifndef NOT_MPI_STRATEGY
    tbb::atomic<int> num_mpi = tbb::atomic<int>();
#define NUM_MPI 50
#endif
///*
#ifdef USE_STEALING
    generic_scheduler::generic_scheduler() : affinity_cpu_id(NO_AFFINITY), my_random(unsigned(this - (generic_scheduler *)NULL))
    {
        id = index++;
        stolen_tol_times = 0;
    }
#else
    generic_scheduler::generic_scheduler() : affinity_cpu_id(NO_AFFINITY)
    {
        id = index++;
        stolen_tol_times = 0;
    }
#endif

#ifdef USE_STEALING
    generic_scheduler::generic_scheduler(int cpu_id) : affinity_cpu_id(cpu_id), my_random(unsigned(this - (generic_scheduler *)NULL))
    {
        id = index++;
        stolen_tol_times = 0;
    }
#else
    generic_scheduler::generic_scheduler(int cpu_id) : affinity_cpu_id(cpu_id)
    {
        id = index++;
        stolen_tol_times = 0;
    }
#endif


//*/
/*
#ifdef USE_STEALING_NUMA
    generic_scheduler::generic_scheduler() : affinity_cpu_id(NO_AFFINITY), my_random(unsigned(this - (generic_scheduler *)NULL))
    {
        id = index++;
        stolen_tol_times = 0;
    }
#else
    generic_scheduler::generic_scheduler() : affinity_cpu_id(NO_AFFINITY)
    {
        id = index++;
        stolen_tol_times = 0;
    }
#endif

#ifdef USE_STEALING_NUMA
    generic_scheduler::generic_scheduler(int cpu_id) : affinity_cpu_id(cpu_id), my_random(unsigned(this - (generic_scheduler *)NULL))
    {
//		std::cout <<"affinity_cpu_id设置成功："<<cpu_id<< std::endl;
        id = index++;
        stolen_tol_times = 0;
    }
#else
    generic_scheduler::generic_scheduler(int cpu_id) : affinity_cpu_id(cpu_id)
    {
        id = index++;
        stolen_tol_times = 0;
    }
#endif
*/
#ifdef	USE_STEALING_SHARED
	concurrent_aceMesh_stack generic_scheduler::shared_queue[NUMA_num];
#endif

#ifdef USE_SHARED_QUEUE
    tbb::concurrent_queue<task *> generic_scheduler::shared_queue;
#endif

#ifdef USE_SHARED_STACK
    concurrent_aceMesh_stack generic_scheduler::shared_queue;
#endif

    generic_scheduler::~generic_scheduler()
    {
    }

#ifdef USE_MY_OPT_STRATEGY
    task *generic_scheduler::copy_data()
    {
        task *cur = NULL;
        if (!my_data_queue.try_pop(cur))
            return NULL;
        task *pre = cur;
        while (my_data_queue.try_pop(cur))
        {
            private_queue.push(pre);
            pre = cur;
        }
        return pre;
    }
#endif

    task *generic_scheduler::get_next_task()
    {
        task *t = NULL;
#ifndef NOT_MPI_STRATEGY
#ifdef BLOCKING_QUEUE               //add wangm
        if ((num_mpi++) == NUM_MPI) //every NUM_MPI times
        {
            num_mpi = 0;
            if (blocking_queue.try_pop(t))
            {
                //std::cout<<"num_mpi times:"<<m++<<std::endl;
                return t;
            }
        }
#endif
#endif
        if (private_queue.try_pop(t))
        {

            return t;
        }
#ifdef USE_STEALING_SHARED
        if (generic_scheduler::shared_queue[id/cores].try_pop(t))
		{
			//std::cout<<"进入下一个队列"<<id%NUMA_num<<std::endl;
            return t;
		}
#endif
#ifdef USE_SHARED_QUEUE
        if (generic_scheduler::shared_queue.try_pop(t))
            return t;
            //std::cout<<"shared->"<<*t<<std::endl;
#endif
#ifdef USE_SHARED_STACK
        if (generic_scheduler::shared_queue.try_pop(t))
            return t;
#endif

#ifdef USE_STEALING
        if (num_threads < 2)
            return NULL;

        //++stolen_tol_times;
        //if(stolen_tol_times % 16 != 0 )
        //{
        //    return NULL;
        //}

        //do stealing
        int x = my_random.get();
        x = x % (num_threads - 1);
        if (x >= id)
            ++x;
//	std::cout <<"本调度器id:"<<id<<" 要窃取的id： "<<x<< std::endl;
        generic_scheduler *tmp = generic_scheduler::schedulers[x];
        if (tmp == NULL)
            return NULL;
        t = tmp->stolen_task();

#ifdef ACEMESH_SCHEDULER_PROFILING
        //do some thing, flag with stolen
        if (t != NULL)
            t->extra_state |= 0x80;
#endif

        return t;
#endif

#ifdef USE_STEALING_NUMA
		if (num_threads < 2)
					return NULL;
		
				//++stolen_tol_times;
				//if(stolen_tol_times % 16 != 0 )
				//{
				//	  return NULL;
				//}
		
				//do stealing
				//std::cout <<"进入USE_STEALING_NUMA"<< std::endl;
				int x =my_random.getnum(id,num_threads);//范围为本NUMA节点内,传入本线程号和总线程数
			//	std::cout <<"本调度器id:"<<id<<" 要窃取的id： "<<x<< std::endl;
				generic_scheduler *tmp = generic_scheduler::schedulers[x];
				if (tmp == NULL)
					return NULL;
				t = tmp->stolen_task();
				//std::cout <<"结束USE_STEALING_NUMA"<< std::endl;
#ifdef ACEMESH_SCHEDULER_PROFILING
				//do some thing, flag with stolen
				if (t != NULL)
					t->extra_state |= 0x80;
#endif
		
				return t;
#endif
        //t = NULL;
        return NULL;
    }

#ifdef MTEST_LIGHT
    mtest_task *generic_scheduler::get_next_mtest_task()
    {
        mtest_task *t = NULL;

        if (polling_queue.try_pop(t))
        {
            return t;
        }
        return NULL;
    }

#endif

    //input scheduler
    void *generic_scheduler::worker_kernel_func(void *p)
    {
        generic_scheduler *my_scheduler = (generic_scheduler *)p;
        theTLS.set(my_scheduler);
//modify CPUBIND by gxr
#ifdef CPUBIND
        if (my_scheduler->affinity_cpu_id != NO_AFFINITY)
        {
            //set affinty
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(my_scheduler->affinity_cpu_id, &mask);
            if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
            {
                std::cout << "pthread_setaffinity_np failed";
            }
            //if (my_mpi_rank < 1)
                //std::cout << "set thread affinity to cpu_id : " << my_scheduler->affinity_cpu_id << std::endl;
            //set end
        }
#endif

#ifdef __ACEMESH_THREAD_GROUP
        if (my_scheduler->ob)
            my_scheduler->ob->on_scheduler_entry(true);
#endif

        while (!is_run)
            ;
        //#ifdef ACEMESH_PERFORMANCE
        //    aceMesh_performance aceMesh_perf;
        //    aceMesh_perf.thread_init();
        //    //aceMesh_perf.record_start();
        //#endif

        task *t = NULL;

        //int my_nums = 0;
        while (!close_all_threads)
        {
            //while( !is_go )
            //{
            //    ;
            //}
            //if( close_all_threads ) break;

            //#ifdef ACEMESH_PERFORMANCE
            //    aceMesh_perf.record_start();
            //#endif
            //while( is_go )
            //{
            if (t == NULL)
            {
#ifdef USE_MY_OPT_STRATEGY
                if ((t = my_scheduler->copy_data()) == NULL)
#endif
                    t = my_scheduler->get_next_task();
            }
            if (t != NULL)
            {
                task *tmp = t;

#ifndef NOT_MPI_STRATEGY
#ifdef EXTRA_THREAD
                if (t && ((aceMesh_task *)t)->get_task_type() == BLOCKING_TASK)
                {
                    task::mpi_spawn(*t);
                    t = NULL;
                }
#endif
#endif

                if (t != NULL)
                {
#ifdef ACEMESH_TIME
                    t->record_execute_time();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
                    t->record_papi_perf_cnt();
#endif
                    task *&local_cur_task = cur_task.local();
                    local_cur_task = t;

                    t = t->execute();

#ifdef NOT_MPI_STRATEGY
                    if (!(tmp->get_reused_flag()))
                    {
                        delete tmp;
                    }
#else

#ifdef BLOCKING_QUEUE
                    if (tmp && !(tmp->get_reused_flag()))
                    {
                        if (tmp && ((aceMesh_task *)tmp)->get_task_type() == BLOCKING_TASK)
                        {
                            if (!((aceMesh_task *)tmp)->get_suspend())
                            {
                                delete tmp;
                            }
                        }
                        else
                        {
                            delete tmp;
                        }
                    }
#endif

#endif
                }
            }
        }

        return NULL;
    }

#ifdef EXTRA_THREAD //add wangm, modify by gxr
    void *generic_scheduler::mpi_worker_kernel_func(void *p)
    {
        generic_scheduler *my_scheduler = (generic_scheduler *)p;
        theTLS.set(my_scheduler);

#ifdef CPUBIND
        if (my_scheduler->affinity_cpu_id != NO_AFFINITY)
        {
            //set affinty
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(my_scheduler->affinity_cpu_id, &mask);
            if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
            {
                std::cout << "pthread_setaffinity_np failed";
            }
            //if (my_mpi_rank < 1)
                //std::cout << "set thread affinity to cpu_id : " << my_scheduler->affinity_cpu_id << std::endl;
            //set end
        }
#endif

#ifdef __ACEMESH_THREAD_GROUP
        if (my_scheduler->ob)
            my_scheduler->ob->on_scheduler_entry(true);
#endif

        while (!is_run)
            ;

        task *t = NULL;

#ifdef MTEST_LIGHT
        int *request1, *request2;
        int flag, flag1, flag2;
        MPI_Status status1, status2;
        mtest_task *mt;
        mtest_task *old_mt;
        int test_count, test_max_count;
#endif

        //int my_nums = 0;
        while (!close_all_threads)
        {
            //polling loop
#ifdef MTEST_LIGHT
            if (t == NULL)
            {
                mt = my_scheduler->get_next_mtest_task();
                old_mt = NULL;
                while (mt != NULL)
                {
                    request1 = mt->comm_handle1;
                    flag1 = 0;
                    if (request1 != NULL)
                    {
                        int rest = 0;
                        test_count = 0;
                        if (mt->comm_kind1 != 3)
                        {
                            test_max_count = 1;
                        }
                        else
                        {
                            test_max_count = 8;
                        }
                        while (test_count < test_max_count && rest == 0)
                        {
                            MPI_Test(request1, &rest, &status1);
                            test_count++;
                        }
                        //#define MPI_SUCCESS          0      /* Successful return code */
                        //if(rest==MPI_SUCCESS) flag1=1;
                        if (rest != 0)
                            flag1 = 1;
                    }
                    else
                    {
                        flag1 = 1;
                    }

                    request2 = mt->comm_handle2;
                    flag2 = 0;
                    if (request2 != NULL)
                    {

                        int rest = 0;
                        test_count = 0;
                        if (mt->comm_kind1 != 3)
                        {
                            test_max_count = 1;
                        }
                        else
                        {
                            test_max_count = 8;
                        }
                        while (test_count < test_max_count && rest == 0)
                        {
                            MPI_Test(request2, &rest, &status2);
                            test_count++;
                        }
                        //if(rest==MPI_SUCCESS) flag2=1;
                        if (rest != 0)
                            flag2 = 1;
                    }
                    else
                    {
                        flag2 = 1;
                    }

                    flag = (flag1 == 1) && (flag2 == 1);
                    if (flag == 0)
                    { //fill
                        my_scheduler->polling_queue.push(mt);
                        if (old_mt == NULL)
                        {
                            old_mt = mt;
                        }
                    }
                    else
                    {
                        task *temp_t = mt->comm_task;
                        //((aceMesh_task*)temp_t)->set_suspend(0);
                        my_scheduler->mpi_queue.push(temp_t);
                        //(aceMesh_task* (mt->comm_task))->s
                        //temp_t->execute();
                        //delete(temp_t);
                        //delete(mt);
                        //free(mt);
                    }
                    mt = my_scheduler->get_next_mtest_task();

                    if (mt == old_mt)
                    {
                        if (mt != NULL)
                        {
                            my_scheduler->polling_queue.push(mt);
                        }
                        break;
                    }
                }
            }

#endif

            if (t == NULL)
            {
#ifdef USE_MY_OPT_STRATEGY
                if ((t = my_scheduler->copy_data()) == NULL)
#endif
                    //a simple get_next_task for MPI threads.
                    my_scheduler->mpi_queue.try_pop(t);
                //t = my_scheduler->get_next_task();
            }
            if (t != NULL)
            {
                task *tmp = t;
#ifdef ACEMESH_TIME
                t->record_execute_time();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
                t->record_papi_perf_cnt();
#endif
                task *&local_cur_task = cur_task.local();
                local_cur_task = t;

                t = t->execute();
                if (t != NULL)
                {
                    t->restore_ref_count();
                }

                if (tmp && !(tmp->get_reused_flag()))
                {
                    if (tmp && ((aceMesh_task *)tmp)->get_task_type() == BLOCKING_TASK)
                    {
                        if (!((aceMesh_task *)tmp)->get_suspend())
                        {
                            delete tmp;
                        }
                    }
                }
                //++my_nums;
            }
            //}
            //#ifdef ACEMESH_PERFORMANCE
            //    aceMesh_perf.record_end();
            //    aceMesh_perf.record_output();
            //#endif
        }

        //#ifdef ACEMESH_PERFORMANCE
        //aceMesh_perf.record_end();
        //aceMesh_perf.record_output();
        //#endif
        //std::cout << my_scheduler->id << " : " << my_nums << std::endl;

        return NULL;
    }
#endif

    //input scheduler and wait task
    void generic_scheduler::main_kernel_func(task *wait_task)
    {

        //is_go = true;
        generic_scheduler *my_scheduler = generic_scheduler::schedulers[0];

        task *t = NULL;

        while (wait_task->ref_count() > 1)
        {
            if (t == NULL)
            {
#ifdef USE_MY_OPT_STRATEGY
                if ((t = my_scheduler->copy_data()) == NULL)
#endif
                    t = my_scheduler->get_next_task();
            }
            if (t != NULL)
            {
                task *tmp = t;

#ifndef NOT_MPI_STRATEGY
#ifdef EXTRA_THREAD
                if (t && ((aceMesh_task *)t)->get_task_type() == BLOCKING_TASK)
                {
                    task::mpi_spawn(*t);
                    t = NULL;
                }
#endif
#endif
                if (t != NULL)
                {
#ifdef ACEMESH_TIME
                    t->record_execute_time();
#endif

#ifdef ACEMESH_PAPI_PERFORMANCE_EACH_TASK
                    t->record_papi_perf_cnt();
#endif
                    task *&local_cur_task = cur_task.local();
                    local_cur_task = t;

                    t = t->execute();
#ifdef NOT_MPI_STRATEGY
                    if (!(tmp->get_reused_flag()))
                    {
                        delete tmp;
                    }
#else

#ifdef BLOCKING_QUEUE
                    //                if (tmp&& ((aceMesh_task*)tmp)->get_task_type()==BLOCKING_TASK) {
                    //                  if (!((aceMesh_task*)tmp)->get_suspend()){
                    //	                  delete tmp;
                    //                  }
                    //                }
                    //                else {
                    //                  delete tmp;
                    //                }
#endif
#endif
                }
            }
        }
        if (!(wait_task->get_reused_flag()))
        {
            delete wait_task;
        }
    }

#ifdef NOT_MPI_STRATEGY
    void generic_scheduler::spawn_to_id(task *t)
    {
#ifndef USE_DEQUE
        int my_id = t->get_affinity_id();
            //std::cout<<"spawn_to_id_my_id= "<<my_id<<std::endl;
        if (my_id > -1)
        {
#ifdef USE_STEALING_SHARED
			 //std::cout<<"spawn_to_id_进队"<<std::endl;
			 this->enqueue(t,my_id);
#else
            generic_scheduler::schedulers[my_id]->local_spawn(t);
#endif
        }
        else if (my_id == FOLLOW_AFFINITY)
        {
            this->local_spawn(t);
        }
        else if (my_id == NO_SET_AFFINITY)
        {
            this->enqueue(t);
        }
        else
        {
            assert(0);
        }
#else
        int my_id = t->get_affinity_id();

        if (my_id > -1)
        {
#ifdef USE_MY_OPT_STRATEGY
            generic_scheduler::schedulers[my_id]->remote_spawn(t);
#else
            generic_scheduler::schedulers[my_id]->local_spawn(t);
#endif
        }
        else if (my_id == FOLLOW_AFFINITY)
        {
            this->local_spawn(t);
        }
        else if (my_id == NO_SET_AFFINITY)
        {
            this->enqueue(t);
        }
        else
        {
            assert(0);
        }

#endif
    }
#else
    void generic_scheduler::spawn_to_id(task *t)
    {
		std::cout<<"my_id= "<<std::endl;
        bool blocking_flag = (((aceMesh_task *)t)->get_task_type() == BLOCKING_TASK);
#ifndef USE_DEQUE
        int my_id = t->get_affinity_id();
        if (my_id > -1)
        {
            if (!blocking_flag)
            {
                generic_scheduler::schedulers[my_id]->local_spawn(t);
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif
#ifdef BLOCKING_QUEUE //add wangm, modify by gxr
                //generic_scheduler::schedulers[my_id]->mpiqueue(t);
                generic_scheduler::schedulers[my_id]->local_spawn(t);
#endif
            }
        }
        else if (my_id == FOLLOW_AFFINITY)
        {
            if (!blocking_flag)
            {
                this->local_spawn(t);
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif
#ifdef BLOCKING_QUEUE //add wangm  ,modify by gxr
                this->local_spawn(t);
#endif
            }
        }
        else if (my_id == NO_SET_AFFINITY)
        {
            if (!blocking_flag)
            {
                this->enqueue(t);
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif
#ifdef BLOCKING_QUEUE //add wangm,modify by gxr
                this->local_spawn(t);
#endif
            }
        }
        else
        {
            assert(0);
        }
#else
        int my_id = t->get_affinity_id();

        if (my_id > -1)
        {
            if (!blocking_flag)
            {
#ifdef USE_MY_OPT_STRATEGY
                generic_scheduler::schedulers[my_id]->remote_spawn(t);
#else
                generic_scheduler::schedulers[my_id]->local_spawn(t);
#endif
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif
#ifdef BLOCKING_QUEUE //add wangm,modify by gxr
                generic_scheduler::schedulers[my_id]->local_spawn(t);
#endif
            }
        }
        else if (my_id == FOLLOW_AFFINITY)
        {
            if (!blocking_flag)
            {
                this->local_spawn(t);
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif
#ifdef BLOCKING_QUEUE //add wangm,modify by gxr
                this->local_spawn(t);
#endif
            }
        }
        else if (my_id == NO_SET_AFFINITY)
        {
            if (!blocking_flag)
            {
                this->enqueue(t);
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif
#ifdef BLOCKING_QUEUE //add wangm ,modify by gxr
                this->local_spawn(t);
#endif
            }
        }
        else
        {
            assert(0);
        }

#endif
    }
#endif

#ifdef USE_DEQUE
#ifdef USE_MY_OPT_STRATEGY
    void generic_scheduler::remote_spawn(task *t)
    {
        this->my_data_queue.push(t);
    }
#endif
#endif

    void generic_scheduler::local_spawn(task *first)
    {
        //  std::cout<<"local spawn->"<<first<<std::endl;
        private_queue.push(first);
    }

#ifndef NOT_MPI_STRATEGY

#ifdef EXTRA_THREAD
    void generic_scheduler::mpi_local_spawn(task *first)
    {
        //  std::cout<<"local spawn->"<<first<<std::endl;
        mpi_queue.push(first);
    }
#ifdef MTEST_LIGHT
    void generic_scheduler::mpi_polling_local_spawn(task *first)
    {
        //mtest_task* my_mtest_task = new mtest_task(comm1,comm2,first);
        //std::cout << "mpi_polling_local_spawn:"<<comm1<<comm2<<std::endl;
        mtest_task *my_mtest_task = new mtest_task();
        my_mtest_task->comm_handle1 = comm1;
        my_mtest_task->comm_handle2 = comm2;
        my_mtest_task->comm_kind1 = mkind1;
        my_mtest_task->comm_kind2 = mkind2;
        my_mtest_task->comm_task = first;
        polling_queue.push(my_mtest_task);
        comm1 = NULL;
        comm2 = NULL;
        //
    }
#endif

#endif
#ifdef BLOCKING_QUEUE //add wangm
    void generic_scheduler::mpiqueue(task *t)
    {
        blocking_queue.push(t);
    }
#endif
#endif

#ifdef NOT_MPI_STRATEGY
    void generic_scheduler::init_spawn(task *t)
    {
        int my_id = t->get_affinity_id();
        if (my_id > -1)
        {

#if defined(USE_MY_OPT_STRATEGY)
            generic_scheduler::schedulers[my_id]->remote_spawn(t);
#elif defined(USE_STEALING_SHARED)
			 //std::cout<<"init_spawn_进队"<<std::endl;
			 this->enqueue(t,my_id);
#else
            generic_scheduler::schedulers[my_id]->local_spawn(t);
#endif
        }
        else if (my_id == FOLLOW_AFFINITY)
        {
            this->local_spawn(t);
        }
        else if (my_id == NO_SET_AFFINITY)
        {
            this->enqueue(t);
        }
        else
        {
            assert(0);
        }
    }
#else
    void generic_scheduler::init_spawn(task *t)
    {
        bool blocking_flag = (((aceMesh_task *)t)->get_task_type() == BLOCKING_TASK);
        // std::cout<<"init spawn->"<<t<<std::endl;
        int my_id = t->get_affinity_id();
        //std::cout<<"my_id->"<<my_id<<std::endl;
        if (my_id > -1)
        {
            if (!blocking_flag)
            {
#ifdef USE_MY_OPT_STRATEGY
                generic_scheduler::schedulers[my_id]->remote_spawn(t);
#else
                generic_scheduler::schedulers[my_id]->local_spawn(t);
#endif
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif

#ifdef BLOCKING_QUEUE //modify by gxr
                generic_scheduler::schedulers[my_id]->local_spawn(t);
#endif
            }
        }
        else if (my_id == FOLLOW_AFFINITY)
        {
            if (!blocking_flag)
            {
                this->local_spawn(t);
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif

#ifdef BLOCKING_QUEUE //modify by gxr
                this->local_spawn(t);
#endif
            }
        }
        else if (my_id == NO_SET_AFFINITY)
        {
            if (!blocking_flag)
            {
                //this->local_spawn(t);
                this->enqueue(t);
            }
            else
            {
#ifdef EXTRA_THREAD //add wangm
                generic_scheduler::mpi_scheduler->mpi_local_spawn(t);
#endif

#ifdef BLOCKING_QUEUE //modify by gxr
                this->local_spawn(t);
#endif
            }
        }
        else
        {
            assert(0);
        }
    }
#endif

    void generic_scheduler::enqueue(task *t)
    {
#if defined(USE_SHARED_QUEUE)
        generic_scheduler::shared_queue.push(t);
        //std::cout<<"enqueue->"<<*t<<std::endl;
#elif defined(USE_STEALING_SHARED)
		generic_scheduler::shared_queue[t->get_affinity_id()%NUMA_num].push(t);
#elif defined(USE_SHARED_STACK)
        generic_scheduler::shared_queue.push(t);
#else
        assert(0);
        //private_queue.push(t);
#endif
    }
	
	
void generic_scheduler::enqueue(task *t,int task_id)
    {

#if defined(USE_STEALING_SHARED)
		generic_scheduler::shared_queue[(task_id%num_threads)/cores].push(t);
		//std::cout<<"进入公有队列id："<<(task_id%num_threads)/cores<<std::endl;
#else
        assert(0);
#endif
    }


#ifdef USE_STEALING_NUMA
    task *generic_scheduler::stolen_task()
    {
#ifdef USE_DEQUE
        return private_queue.take();
#else
        task *tmp = NULL;
        if (private_queue.try_pop(tmp))
            return tmp;
        return NULL;
#endif
    }
#endif

#ifdef USE_STEALING
    task *generic_scheduler::stolen_task()
    {
#ifdef USE_DEQUE
        return private_queue.take();
#else
        task *tmp = NULL;
        if (private_queue.try_pop(tmp))
            return tmp;
        return NULL;
#endif
    }
#endif

    pthread_t generic_scheduler::new_thread(void *closure)
    {
        pthread_t thread_handle;
        int status;
        /*
    pthread_attr_t stack_size;
    status = pthread_attr_init( &stack_size );
    if( status )
        return 0;
    status = pthread_attr_setstacksize( &stack_size, ThreadStackSize );
    if( status )
        return 0;
    */
        status = pthread_create(&thread_handle, NULL, worker_kernel_func, closure);
        if (status)
            return 0;
        return thread_handle;
    }

#ifdef EXTRA_THREAD //ADD WANGM,
    pthread_t generic_scheduler::new_mpi_thread(void *closure)
    {
        pthread_t thread_handle;
        int status;
        /*
    pthread_attr_t stack_size;
    status = pthread_attr_init( &stack_size );
    if( status )
        return 0;
    status = pthread_attr_setstacksize( &stack_size, ThreadStackSize );
    if( status )
        return 0;
    */
        status = pthread_create(&thread_handle, NULL, mpi_worker_kernel_func, closure);
        assert(!status);
        //if( status )
        //    return 0;
        return thread_handle;
    }
#endif
    void auto_do_exit(void *p)
    {

        //std::cout << "do some thing:" << p << std::endl;
        //delete static_cast<generic_scheduler*>(p);
    }

    void generic_scheduler::create_worker_thread(int n_threads)
    {
        sche_num_threads = n_threads;
        //    std::cout<<"SCHE_NUM_THREAD"<<sche_num_threads<<std::endl;
        std::vector<generic_scheduler *> tmp(n_threads, NULL);
        schedulers.swap(tmp);
        std::vector<pthread_t> tmp_threads(n_threads, NULL);
        threads.swap(tmp_threads);

        generic_scheduler *my_scheduler = new generic_scheduler();
        generic_scheduler::schedulers[0] = my_scheduler;
        theTLS.create(auto_do_exit);
        theTLS.set(my_scheduler);

        is_run = false;
        generic_scheduler::num_threads = n_threads;
        for (int i = 1; i < n_threads; ++i)
        {
            generic_scheduler::schedulers[i] = new generic_scheduler();
            threads[i] = new_thread((void *)generic_scheduler::schedulers[i]);
            //threads.push_back( new_thread( (void*)generic_scheduler::schedulers[i] ));
        }
#ifdef EXTRA_THREAD //add wangm
        generic_scheduler::mpi_scheduler = new generic_scheduler();
        mpi_thread = new_mpi_thread((void *)(generic_scheduler::mpi_scheduler));
#endif
        is_run = true;
    }
    void generic_scheduler::create_worker_thread(int n_threads, int p[])
    {
        sche_num_threads = n_threads;
        std::vector<generic_scheduler *> tmp(n_threads, NULL);
        schedulers.swap(tmp);
        std::vector<pthread_t> tmp_threads(n_threads, NULL);
        threads.swap(tmp_threads);

        generic_scheduler *my_scheduler = new generic_scheduler(p[0]);
        generic_scheduler::schedulers[0] = my_scheduler;
        theTLS.create(auto_do_exit);
        theTLS.set(my_scheduler);

        //set master affinity
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(p[0], &mask);

        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        {
            std::cout << "pthread_setaffinity_np failed";
        }
        //if (my_mpi_rank < 1)
            //std::cout << "set thread affinity to cpu_id : " << my_scheduler->affinity_cpu_id << std::endl;
        //set end
        is_run = false;

        generic_scheduler::num_threads = n_threads;
            //std::cout <<"generic_scheduler::num_threads"<<generic_scheduler::num_threads<< std::endl;
        for (int i = 1; i < n_threads; ++i)
        {
            generic_scheduler::schedulers[i] = new generic_scheduler(p[i]);
			//std::cout <<"generic_scheduler::num_threads"<<p[i]<< std::endl;
            threads[i] = new_thread((void *)generic_scheduler::schedulers[i]);
            //threads.push_back( new_thread( (void*)generic_scheduler::schedulers[i] ));
        }
#ifdef EXTRA_THREAD //add wangm
        generic_scheduler::mpi_scheduler = new generic_scheduler(p[n_threads]);
        mpi_thread = new_mpi_thread((void *)(generic_scheduler::mpi_scheduler));
#endif
        is_run = true;
    }

    void generic_scheduler::close_worker_thread()
    {
        close_all_threads = true;
        //is_go = true;
        for (int i = 1; i < generic_scheduler::num_threads; ++i)
        {
            //std::cout << "s2: " << generic_scheduler::schedulers[i] << std::endl;
            //generic_scheduler::schedulers[i]->close = true;
            pthread_join(generic_scheduler::threads[i], NULL);
            delete generic_scheduler::schedulers[i];
        }
#ifdef EXTRA_THREAD //add wangm,
        pthread_join(generic_scheduler::mpi_thread, NULL);
        delete generic_scheduler::mpi_scheduler;

#endif

        delete generic_scheduler::schedulers[0];
        theTLS.destroy();
    }

#ifdef __ACEMESH_THREAD_GROUP
    void generic_scheduler::create_worker_thread(aceMesh_observer *ob, int n_threads, int p[])
    {
        sche_num_threads = n_threads;
        std::vector<generic_scheduler *> tmp(n_threads, NULL);
        schedulers.swap(tmp);
        std::vector<pthread_t> tmp_threads(n_threads, NULL);
        threads.swap(tmp_threads);

        generic_scheduler *my_scheduler = new generic_scheduler();
        my_scheduler->ob = ob;
        if (ob)
            ob->on_scheduler_entry(false);

        generic_scheduler::schedulers[0] = my_scheduler;
        theTLS.create(auto_do_exit);
        theTLS.set(my_scheduler);

        is_run = false;

        generic_scheduler::num_threads = n_threads;
        for (int i = 1; i < n_threads; ++i)
        {
            generic_scheduler::schedulers[i] = new generic_scheduler();
            generic_scheduler::schedulers[i]->ob = ob;
            threads[i] = new_thread((void *)generic_scheduler::schedulers[i]);
            //threads.push_back( new_thread( (void*)generic_scheduler::schedulers[i] ));
        }
        is_run = true;
    }
#endif

} // namespace AceMesh_runtime
