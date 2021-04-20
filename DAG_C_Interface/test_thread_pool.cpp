#include <iostream>

class test_job : public task 
{
public:
    task* execute()
    {
        cout << "test\n";
        return NULL;
    }
};

int main()
{
    for(int i = 0; i < 4; ++i)
        create_worker_thread();
    test_job j1 = new  test_job();
    test_job j2 = new  test_job();
    test_job j3 = new  test_job();
    test_job j4 = new  test_job();
    generic_scheduler::schedulers[0].local_spawn(j1);
    generic_scheduler::schedulers[1].local_spawn(j2);
    generic_scheduler::schedulers[2].local_spawn(j3);
    generic_scheduler::schedulers[3].local_spawn(j4);
    wait_for_all();
    return 0;
}