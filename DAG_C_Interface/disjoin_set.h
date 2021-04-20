#ifndef _DISJOIN_SET_H
#define _DISJOIN_SET_H

#include <map>
#include <vector>

namespace AceMesh_runtime {
class disjoin_set
{
    private:
        int* father;
        int* rank;
        int count;
        int my_size;
    public:
        disjoin_set(int max_n);
        ~disjoin_set();
        void clear();
        void make_set(int x);
        int  find_set(int x);
        int  union_set(int x, int y);
        int  get_count();
        int get_sep_data(std::map<int,std::vector<int> >& sep_data);
};
}

#endif
