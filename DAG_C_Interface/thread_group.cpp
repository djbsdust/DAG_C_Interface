#include "thread_group.h"
#include <cassert>
#include <pthread.h>

namespace AceMesh_runtime {
thread_group::thread_group()
{

}

thread_group::~thread_group()
{
	subthreads_run = false;
	for (size_t i = 0; i < group_size; ++i)
	{
		pthread_mutex_lock(&subthread_control_structs[i].cond_wait_mutex);
		pthread_cond_signal(&subthread_control_structs[i].cond_go);
		pthread_mutex_unlock(&subthread_control_structs[i].cond_wait_mutex);
	}
	
}

void thread_group::init(size_t group_id, size_t group_size, const std::vector<size_t> & core_ids)
{
	//assert(core_ids.size() >= group_size);

	this->group_id = group_id;
	this->group_size = group_size;
    if(group_size <= 0) return ;


	subthread_control_structs.resize(group_size);

	for (size_t i = 0; i < group_size; ++i)
	{
		subthread_control_structs[i].my_core_id = core_ids[i];
		pthread_cond_init(&subthread_control_structs[i].cond_go,NULL);
		pthread_cond_init(&subthread_control_structs[i].cond_finish,NULL);
		subthread_control_structs[i].is_idle = true;
		pthread_mutex_init(&subthread_control_structs[i].cond_wait_mutex,NULL);

		// subthread_control_structs[i].my_id = i;
		subthread_control_structs[i].p_my_group = this;

	}
}

void* subthread_func(void* arg);

void thread_group::start()
{
	subthreads_run = true;
	for(size_t i = 0; i < group_size; ++i) 
	{
		int status = pthread_create(&subthread_control_structs[i].my_pthread_id, NULL, subthread_func, &(subthread_control_structs[i]));
		if(status)
			exit(status);
	}
}


void thread_group::run_task(size_t subthread_index, hierarchy_task_base* p_task, const void* p_param)
{
	subthread_control_structs[subthread_index].p_param = p_param;
	subthread_control_structs[subthread_index].p_task = p_task;

	pthread_mutex_lock(&subthread_control_structs[subthread_index].cond_wait_mutex);
	subthread_control_structs[subthread_index].is_idle = false;
	pthread_cond_signal(&subthread_control_structs[subthread_index].cond_go);
	pthread_mutex_unlock(&subthread_control_structs[subthread_index].cond_wait_mutex);
}

void thread_group::run_task(size_t subthread_index)
{
	pthread_mutex_lock(&subthread_control_structs[subthread_index].cond_wait_mutex);
	subthread_control_structs[subthread_index].is_idle = false;
 	pthread_cond_signal(&subthread_control_structs[subthread_index].cond_go);
 	pthread_mutex_unlock(&subthread_control_structs[subthread_index].cond_wait_mutex);
}


void thread_group::wait_all_tasks()
{
	for (size_t i = 0; i < group_size; ++i)
	{
		pthread_mutex_lock(&subthread_control_structs[i].cond_wait_mutex);
		if(!subthread_control_structs[i].is_idle)
		{
			pthread_cond_wait(&subthread_control_structs[i].cond_finish, &subthread_control_structs[i].cond_wait_mutex);
		}
		pthread_mutex_unlock(&subthread_control_structs[i].cond_wait_mutex);
	}
}

/*
void thread_group::run_task_sync(size_t subthread_index)
{
	subthread_tasks[subthread_index]->execute_task_body(subthread_task_params[subthread_index]);
}
*/

size_t thread_group::get_group_size()
{
	return group_size;
}

size_t thread_group::get_group_id()
{
	return group_id;
}


void thread_group::set_task(size_t subthread_index, hierarchy_task_base* p_task)
{
	subthread_control_structs[subthread_index].p_task = p_task;
}

void thread_group::set_task_param(size_t subthread_index, const void * p_param)
{
	subthread_control_structs[subthread_index].p_param = p_param;
}


void* subthread_func(void* arg)
{

	thread_group::thread_control_struct* my_control_struct = (thread_group::thread_control_struct*)arg;

	const thread_group* p_my_group = my_control_struct->p_my_group;
//	size_t my_id = my_control_struct->my_id;
	size_t my_core_id = my_control_struct->my_core_id;
	
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	CPU_SET(my_core_id, &cpu_set);
	sched_setaffinity(0,sizeof(cpu_set_t),&cpu_set);

	while(p_my_group->subthreads_run)
	{
		pthread_mutex_lock(&my_control_struct->cond_wait_mutex);
		while(my_control_struct->is_idle == true)
		{
	    	pthread_cond_wait(&my_control_struct->cond_go, &my_control_struct->cond_wait_mutex);
		}
		pthread_mutex_unlock(&my_control_struct->cond_wait_mutex);

		assert(my_control_struct->p_task != 0 && my_control_struct->p_param != 0);

		my_control_struct->p_task->execute_task_body(my_control_struct->p_param);


		pthread_mutex_lock(&my_control_struct->cond_wait_mutex);
		my_control_struct->is_idle = true;
		pthread_cond_signal(&my_control_struct->cond_finish);
		pthread_mutex_unlock(&my_control_struct->cond_wait_mutex);


	}

	return NULL;
}
}
