#include <iostream>
#include "aceMesh_runtime.h"
using namespace std;


class A : public aceMesh_task {
    virtual task* execute()
    {
        cout << "just test!\n";
        return aceMesh_task::execute();
    }
};
int main()
{
    begin_split_task();
    A a1 = A();
    A a2 = A();
    int addr;
    register_task(&a1, 1, &addr, NORMAL, NEIGHBOR_NONE, IN, INOUT_NONE);
    register_task(&a2, 1, &addr, NORMAL, NEIGHBOR_NONE, OUT, INOUT_NONE);
    end_split_task();
    return 0;
}
