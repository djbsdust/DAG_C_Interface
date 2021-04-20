#include "affinity_id_generator.h"
#include "aceMesh_utils.h"
#include "scheduler_common.h"
#include <iostream>

namespace AceMesh_runtime {
affinity_id_generator::affinity_id_generator(int thread_nums):id(0), thread_num(thread_nums)
{
}

affinity_id_generator::affinity_id_generator():id(0)
{
}
affinity_id_generator::~affinity_id_generator()
{
}

int affinity_id_generator::get_affinity_id()
{
    int res = id;
    id = ++id % thread_num;
    return res;

}

void affinity_id_generator::set_thread_num(int thread_nums)
{
    thread_num = thread_nums;
}

void affinity_id_generator::reset_affinity_id()
{
    id = 0;
}
}
