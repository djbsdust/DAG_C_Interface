#include "tbb/atomic.h"
#include <iostream>
#include <cstring>

namespace AceMesh_runtime {
template<class T>
class circular_array
{
public:
    circular_array(unsigned long long capacity)
    {
        this->array = new T [capacity];
        this->my_size = capacity;
    }
    ~circular_array()
    {
        delete [] this->array;
    }

    void set(unsigned long long index, T t)
    {
        this->array[index % this->my_size] = t;
    }
    T get(unsigned long long index)
    {
        return this->array[index % this->my_size];
    }
    circular_array* get_double_sized_copy(unsigned long long b,  unsigned long long t)
    {
        circular_array* new_arr;
        if (this->my_size * 2 >= (1 << 31))
        {
            std::cerr << "gsoc_task_circular_array cannot deal with more than 2^31 tasks.\n" ;
            exit(1);
        }
        std::cout << "new : " << this->my_size * 2 << std::endl;
        new_arr = new circular_array(this->my_size * 2);

        //for(unsigned long long i = t; i < b; ++i)
        //{
        //    new_arr->set(i, this->get(i));
        //}
        memcpy(&new_arr->array[0], &this->array[0], sizeof(T) * this->my_size);
        return new_arr;
    }

    T* array;
    unsigned long long my_size;
};
}
