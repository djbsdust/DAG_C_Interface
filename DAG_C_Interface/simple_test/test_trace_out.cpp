#include <iostream>
#include "aceMesh_runtime.h"

using namespace std;

int main()
{
    print_to_thread_file("my test %d,test:%d",0,100);
    return 0;
}
