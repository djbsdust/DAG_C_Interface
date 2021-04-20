#ifndef _TRACE_OUT_H
#define _TRACE_OUT_H
//interface 
//

#include <fstream>
#include <string>
#include "tbb/concurrent_unordered_map.h"
#include "tbb/atomic.h"

namespace AceMesh_runtime {
class thread_ctl 
{
private:
    std::string out_filename;
    struct thread_private_info
    {
        std::ofstream* outfile;
        //other info
    };
    tbb::concurrent_unordered_map<unsigned int, thread_private_info> thread_info;

public:
    thread_ctl(std::string out_filename="trace_"); 
    ~thread_ctl();
    void init(std::string mydir);
    void end_file_spawn();
    std::ofstream* get_file();
    void print(const char* format, ... );
    void print_long_long(long long data);
};


std::ofstream* get_file();
//old interface
void end_file_spawn();

void print_to_internal_thread_file(const char* format, ... );
void print_long_long_thread_file(long long data);
void init_trace_ctl(const std::string& mydir);
}
#endif
