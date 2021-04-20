#ifndef AFFINITY_ID_GENERATOR_H
#define AFFINITY_ID_GENERATOR_H

namespace AceMesh_runtime {
//TODO
#define CYCILC 0

class affinity_id_generator
{
public:
    affinity_id_generator(int thread_nums);
    affinity_id_generator();
    ~affinity_id_generator();
    int get_affinity_id();
    void reset_affinity_id();
    void set_thread_num(int thread_nums);
private:
    int id;
    int thread_num;
};
}

#endif
