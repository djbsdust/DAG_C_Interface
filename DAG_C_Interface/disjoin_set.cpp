#include "disjoin_set.h"

namespace AceMesh_runtime {
disjoin_set::disjoin_set(int max_n)
{
    father = new int [max_n];
    rank = new int [max_n];
    count = 0;
    my_size = 0; 
}


disjoin_set::~disjoin_set()
{
    delete [] father;
    delete [] rank;
}

void disjoin_set::clear()
{
    count = 0;
    my_size = 0;
}
void disjoin_set::make_set(int x)
{
    father[x] = x;
    rank[x] = 0;
    ++count;
    ++my_size;
}

int disjoin_set::find_set(int x)
{
    if(x != father[x])
    {
        father[x] = find_set(father[x]);
    }
    return father[x];
}

int disjoin_set::union_set(int x, int y)
{
    x = find_set(x);
    y = find_set(y);
    if(x == y)
        return x;

    if(rank[x] > rank[y])
    {
        father[y] = x;
        --count;
        return x;
    }
    else 
    {
        if(rank[x] == rank[y])
        {
            rank[y]++;
        }
        father[x] = y;
        --count;
        return y;
    }
}

int disjoin_set::get_count()
{
    return count;
}



int disjoin_set::get_sep_data(std::map<int,std::vector<int> >& sep_data)
{
    sep_data.clear();
    std::map<int, std::vector<int> >::iterator itr;
    for(int i = 0; i < my_size; ++i)
    {
        int x = find_set(i);
        if( (itr = sep_data.find(x) ) != sep_data.end())
        {
            sep_data[x].push_back(i);
        }
        else
        {
            std::vector<int> data;
            data.push_back(i);
            sep_data.insert(std::pair<int,std::vector<int> >(x, data));
        }
    }
    return count;
}
}
