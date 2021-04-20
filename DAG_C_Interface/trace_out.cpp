#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>

#include "trace_out.h"
#include "aceMesh_runtime.h"
#include "scheduler.h"

using namespace std;
using namespace tbb;

namespace AceMesh_runtime {
thread_ctl::thread_ctl(string out_filename) : out_filename(out_filename){
}


thread_ctl::~thread_ctl()
{
    for(concurrent_unordered_map<unsigned int, thread_private_info>::iterator itr = thread_info.begin();
        itr != thread_info.end(); ++itr)
    {
        itr->second.outfile->close();
        delete itr->second.outfile;
    }
}
void thread_ctl::init(std::string mydir){

  std::string prefix="";
#ifdef ACEMESH_SCHEDULER_PROFILING_EACH_TASK
  prefix =  "scheduler_thread_dyn_";
#else
  prefix =  "scheduler_thread_";
#endif

    if(mydir.length() != 0)
	  out_filename = mydir + prefix;
	else 
	  out_filename=prefix;
    return;
}
void thread_ctl::end_file_spawn()
{
    for(concurrent_unordered_map<unsigned int, thread_private_info>::iterator itr = thread_info.begin();
        itr != thread_info.end(); ++itr)
    {
            *(itr->second.outfile)<<"spawn\n";
    }
}

ofstream* thread_ctl::get_file()
{
    unsigned int myid=generic_scheduler::theTLS.get()->get_myid(); //by lchen
    concurrent_unordered_map<unsigned int, thread_private_info>::iterator itr = thread_info.find(myid);
    
	if ( itr == thread_info.end() )
    {
        thread_private_info info;
        stringstream filename;
        filename<<out_filename;
        filename<<myid; //by lchen
        filename<<".csv";
        info.outfile = new ofstream(filename.str().c_str());
        std::pair<unsigned int , thread_private_info> ins(myid, info);
        thread_info.insert(ins);
        return info.outfile;
    }
    else
    {
        return itr->second.outfile;
    }
}

void thread_ctl::print(const char* fmt, ... )
{
    ofstream* out = get_file();

    double vargflt = 0;
    int  vargint = 0;
    char* vargpch = NULL;
    char vargch = 0;
    const char* pfmt = NULL;
    va_list vp;

    va_start(vp, fmt);
    pfmt = fmt;

    while(*pfmt)
    {
        if(*pfmt == '%')
        {
            switch(*(++pfmt))
            {
                case 'c':
                    vargch = va_arg(vp, int); 
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                    *                            mode, or a warning otherwise.In non-strict ANSI mode, 'type' is allowed i
                    *                            to be any expression. */
                    *out<<vargch;
                    break;
                case 'd':
                case 'i':
                    vargint = va_arg(vp, int);
                    *out<<vargint;
                    break;
                case 'f':
                    vargflt = va_arg(vp, double);
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                    *                            mode, or a warning otherwise.In non-strict ANSI mode, 
                    *                            'type' is allowed to be any expression. */
                    *out<<vargflt;
                    break;
                case 's':
                    vargpch = va_arg(vp, char*);
                    *out<<(vargpch);
                    break;
                case 'b':
                case 'B':
                    vargint = va_arg(vp, int);
                    *out<<(vargint);
                    break;
                case 'x':
                case 'X':
                    vargint = va_arg(vp, int);
                    *out<<(vargint);
                    break;
                case '%':
                    *out<<('%');
                    break;
                default:
                    break;
            }
            pfmt++;
        }
        else
        {
            *out<<*pfmt;
            ++pfmt;
        }
   }
   va_end(vp);
}
void thread_ctl::print_long_long(long long data)
{
    ofstream* out = get_file();
    //*out << data << std::endl;
    *out << data ;
}

//only for runtime self
thread_ctl internal_ctl;

void init_trace_ctl(const std::string& mydir){
    internal_ctl.init(mydir);
}

//return ofstream object for output
std::ofstream* get_file()
{
    return internal_ctl.get_file();
}

void print_long_long_thread_file(long long data)
{
    internal_ctl.print_long_long(data);
}

void print_to_internal_thread_file(const char* fmt, ... )
{
    ofstream* out = internal_ctl.get_file();

    double vargflt = 0;
    int  vargint = 0;
    char* vargpch = NULL;
    char vargch = 0;
    const char* pfmt = NULL;
    va_list vp;

    va_start(vp, fmt);
    pfmt = fmt;

    while(*pfmt)
    {
        if(*pfmt == '%')
        {
            switch(*(++pfmt))
            {
                case 'c':
                    vargch = va_arg(vp, int); 
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                    *                            mode, or a warning otherwise.In non-strict ANSI mode, 'type' is allowed i
                    *                            to be any expression. */
                    *out<<vargch;
                    break;
                case 'd':
                case 'i':
                    vargint = va_arg(vp, int);
                    *out<<vargint;
                    break;
                case 'f':
                    vargflt = va_arg(vp, double);
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                    *                            mode, or a warning otherwise.In non-strict ANSI mode, 
                    *                            'type' is allowed to be any expression. */
                    *out<<vargflt;
                    break;
                case 's':
                    vargpch = va_arg(vp, char*);
                    *out<<(vargpch);
                    break;
                case 'b':
                case 'B':
                    vargint = va_arg(vp, int);
                    *out<<(vargint);
                    break;
                case 'x':
                case 'X':
                    vargint = va_arg(vp, int);
                    *out<<(vargint);
                    break;
                case '%':
                    *out<<('%');
                    break;
                default:
                    break;
            }
            pfmt++;
        }
        else
        {
            *out<<*pfmt;
            ++pfmt;
        }
   }
   va_end(vp);
}

//for user
thread_ctl th_ctl;
void print_to_thread_file(const char* fmt, ... )
{
    ofstream* out = th_ctl.get_file();

    double vargflt = 0;
    int  vargint = 0;
    char* vargpch = NULL;
    char vargch = 0;
    const char* pfmt = NULL;
    va_list vp;

    va_start(vp, fmt);
    pfmt = fmt;

    while(*pfmt)
    {
        if(*pfmt == '%')
        {
            switch(*(++pfmt))
            {
                case 'c':
                    vargch = va_arg(vp, int); 
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                    *                            mode, or a warning otherwise.In non-strict ANSI mode, 'type' is allowed i
                    *                            to be any expression. */
                    *out<<vargch;
                    break;
                case 'd':
                case 'i':
                    vargint = va_arg(vp, int);
                    *out<<vargint;
                    break;
                case 'f':
                    vargflt = va_arg(vp, double);
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                    *                            mode, or a warning otherwise.In non-strict ANSI mode, 
                    *                            'type' is allowed to be any expression. */
                    *out<<vargflt;
                    break;
                case 's':
                    vargpch = va_arg(vp, char*);
                    *out<<(vargpch);
                    break;
                case 'b':
                case 'B':
                    vargint = va_arg(vp, int);
                    *out<<(vargint);
                    break;
                case 'x':
                case 'X':
                    vargint = va_arg(vp, int);
                    *out<<(vargint);
                    break;
                case '%':
                    *out<<('%');
                    break;
                default:
                    break;
            }
            pfmt++;
        }
        else
        {
            *out<<*pfmt;
            ++pfmt;
        }
   }
   va_end(vp);
}

void end_file_spawn()
{
    th_ctl.end_file_spawn();
    internal_ctl.end_file_spawn();
}

}
