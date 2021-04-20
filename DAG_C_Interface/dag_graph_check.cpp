#ifdef DEBUG_GRAPH

#include "dag_graph_check.h"
#include <cassert>
#include <queue>
#include <execinfo.h>
#include <cstring>

namespace AceMesh_runtime {
/*dag_graph_check::dag_graph_check():output_filename("loop_dependence.info"),out("loop_dependence.info")
{
}*/


dag_graph_check::~dag_graph_check()
{
    out.close();
}

void dag_graph_check::init(const std::string& out_dir)
{
	if(out_dir.length()!=0)
	  output_filename=out_dir+"loop_dependence.info";
	else 
	  output_filename="loop_dependence.info";
	out.open(output_filename, std::ofstream::out);
    return;
}

void dag_graph_check::add_loop_info(const std::string& loop_name)
{
    this->loop_infos.push_back(loop_name);
    ++this->loop_id;
    this->task_id = 0;
}
std::string dag_graph_check::get_last_loop_info()
{
    if(loop_infos.size() > 0)
    {
        return loop_infos[loop_infos.size() - 1];
    }
    else 
    {
        return "";
    }
}
bool dag_graph_check::bfs(std::vector<aceMesh_task*>& start_tasks)
{
    return true;
}

void dag_graph_check::clear()
{
    this->loop_id = 0;
    this->task_id = 0;
    this->loop_infos.clear();
}
void dag_graph_check::print_id_loop_info_table()
{
    int id = 1;
    for(std::vector<std::string>::iterator itr = loop_infos.begin(); itr != loop_infos.end(); ++itr)
    {
        out << id << ":" << *itr << std::endl; 
        ++id;
    }
}
void dag_graph_check::print_loop_depedence(std::vector<aceMesh_task*>& start_tasks, int task_nums)
{
    this->print_id_loop_info_table();
    this->check_graph_bfs(start_tasks, task_nums, true);
    this->clear();
}

void dag_graph_check::set_task_loop_id(aceMesh_task* t)
{
    t->set_loop_id(this->loop_id);
    t->set_task_id(this->task_id);
    ++this->task_id;
}

bool dag_graph_check::check_graph_dfs(std::vector<aceMesh_task*>& start_tasks, int task_nums)
{
    std::cout<<"task_sum "<<task_nums << std::endl;
    int sum_tasks = task_nums;
    if(task_nums == 0)
    {
        return true;
    }

    ++sum_tasks;
	for(std::vector<aceMesh_task*>::iterator itr = start_tasks.begin(); 
            itr != start_tasks.end(); ++itr)
    {
        assert((*itr)->get_ref_count_for_check() == -1);
        (*itr)->set_ref_count_for_check((*itr)->ref_count());
        this->dfs(*itr, sum_tasks, 0);
        (*itr)->set_ref_count_for_check( -1 );
    }

    if(sum_tasks == 0) 
    {
        std::cout<<"Check Graph OK !!!\n";
    }
    else 
    {
        std::cout<<"There are " << sum_tasks << " tasks that I can't execute" << std::endl;
        std::cout<<"Check Error !!!\n";
    }

    return 0;
}

bool dag_graph_check::check_graph_bfs(std::vector<aceMesh_task*>& start_tasks, int task_nums, bool print_info )
{
    std::cout<<"task_sum "<<task_nums << std::endl;
    if(task_nums == 0)
    {
        out << "#" << std::endl;
        return true;
    }
    std::queue<aceMesh_task*> q;
	for(std::vector<aceMesh_task*>::iterator itr = start_tasks.begin(); 
            itr != start_tasks.end(); ++itr)
    {
        q.push(*itr);
        (*itr)->set_ref_count_for_check ( (*itr)->ref_count() );
    }
    ++task_nums;

    bool res = true;
    while(task_nums > 0)
    {
        if(q.size() == 0)
        {
            std::cout<<"There are " << task_nums << " tasks that I can't execute" << std::endl;
            std::cout<<"Check Error !!!\n";
            res = false;
            assert(0);
            break;
        }

        aceMesh_task* tmp = q.front();
        aceMesh_task* vertical_task = tmp->get_vertical_task();
        bool has_out = false;
        if ( vertical_task != NULL )
        {
            if(  print_info && !vertical_task->is_end_task())
            {
                out << "parent loop id," << tmp->get_loop_id() << 
                    ", parent task id," << tmp->get_task_id() <<
                    ", v_child loop id,"  << vertical_task->get_loop_id() <<
                    ", v_child task id, " << vertical_task->get_task_id() ;
                has_out = true;
            }
            if(vertical_task->get_ref_count_for_check() == -1)
            {
                vertical_task->set_ref_count_for_check( vertical_task->ref_count() );
            }
            else 
            {
                assert(vertical_task->get_ref_count_for_check() > 0);
            }
            if(vertical_task->dec_ref_count_for_check() == 0)
            {
                q.push(vertical_task);
            }
        }
	    for(unsigned int i = 0; i < tmp->get_successor_sum(); ++i)
	    {
		    if(aceMesh_task* it = tmp->get_successor_task(i))
            {
                if(  print_info && !it->is_end_task() )
                {
                    if( has_out )
                        out << ", child loop id,"  << it->get_loop_id() <<
                            ", child task id, " << it->get_task_id() ;
                    else 
                        out << "parent loop id," << tmp->get_loop_id() << 
                            ", parent task id," << tmp->get_task_id() <<
                            ", child loop id,"  << it->get_loop_id() <<
                            ", child task id, " << it->get_task_id() ;
                    has_out = true;
                }
                if(it->get_ref_count_for_check() == -1)
                {
                    it->set_ref_count_for_check ( it->ref_count() );
                }
                else 
                {
                    //std::cout<<"task ref_count :"<< t->ref_count() << std::endl;
                    assert(it->get_ref_count_for_check() > 0);
                }
                if(it->dec_ref_count_for_check() == 0)
                {
                    q.push(it);
                }
            }
        }
        if(print_info &&  has_out )
            out << std::endl;
        else if ( print_info  && ! tmp->is_end_task() )
            out << "parent loop id," << tmp->get_loop_id() << 
                    ", parent task id," << tmp->get_task_id() << std::endl;
        q.pop();
        --task_nums;
        tmp->set_ref_count_for_check( -1 );
    }
    out << "#" << std::endl;
    std::cout << "check graph ok!" << std::endl;
    return res;
}

void dag_graph_check::dfs(aceMesh_task* root, int& task_nums, int deep_length)
{
    if( !root->is_end_task() && root->get_successor_sum() == 0)
    {
        std::cout<<"alone task! in deep length : "<<deep_length <<  std::endl;
        std::cout << "warining! add end to this task"<< std::endl;
        //this->set_vertical_task();
        assert(0);
    }
    --task_nums;
    aceMesh_task* vertical_task = root->get_vertical_task();
    if ( vertical_task != NULL )
    {
        if(vertical_task->get_ref_count_for_check() == -1)
        {
            vertical_task->set_ref_count_for_check ( vertical_task->ref_count() );
        }
        else 
        {
            //std::cout<<"task ref_count :"<< t->ref_count() << std::endl;
            //std::cout<<"houji task  nums:"<< t->get_successor_sum() << std::endl;
            assert(vertical_task->get_ref_count_for_check() > 0);
        }
        if(vertical_task->dec_ref_count_for_check() == 0)
        {
            this->dfs(vertical_task, task_nums, deep_length + 1);
            vertical_task->set_ref_count_for_check( -1 );
        }
    }
	for(unsigned int i = 0; i < root->get_successor_sum(); ++i)
	{
		if(aceMesh_task* tmp = root->get_successor_task(i))
        {
            if(tmp->get_ref_count_for_check() == -1)
            {
                tmp->set_ref_count_for_check( tmp->ref_count() );
            }
            else 
            {
                //std::cout<<"task ref_count :"<< t->ref_count() << std::endl;
                assert(tmp->get_ref_count_for_check() > 0);
            }
            if(tmp->dec_ref_count_for_check() == 0)
            {
                this->dfs(tmp, task_nums, deep_length + 1);
                tmp->set_ref_count_for_check( -1 );
            }
        }
    }
}

void dag_graph_check::print_stack()
{
    const int maxLevel = 200;
    void* buffer[maxLevel];
    int level = backtrace(buffer, maxLevel);

    const int SIZE = 1024;
    char cmd[SIZE] = "addr2line -C -f -e ";
    // let prog point to the end of "cmd"
    char* prog = cmd + strlen(cmd);
    int r = readlink("/proc/self/exe", prog, sizeof(cmd) - (prog-cmd)-1);
    FILE* fp = popen(cmd, "w");
    if (!fp)
    {
        perror("popen");
        return;
    }
    for (int i = 1; i < level; ++i)
    {
        fprintf(fp, "%p\n", buffer[i]);
    }
    fclose(fp);

    /*
    int i;
    char strbuffer[1024];
    int errcode;
    int btnum = 0;
    void *btbuf[100];
    char **btstrings = NULL;
    // Get backtrace 
    btnum = backtrace(btbuf, 100);

    btstrings = backtrace_symbols(btbuf, btnum);
    errcode = errno;
    if (btstrings == NULL) 
    {
        assert(0);
        //sprintf(strbuffer, "ALPCLOSE get backtrace failed. ErrCode: %d, Error description: %s\n",
        //errcode, strerror(errcode));
    } 
    else 
    {
        sprintf(strbuffer, "Backtraces, total %d items\n", btnum);
        std::cout << strbuffer;
        for (i = 0; i < btnum; i++) 
        {
            sprintf(strbuffer, "%s\n", btstrings[i]);
            std::cout << strbuffer;
        }
        free(btstrings);
    }*/
}
}
#endif
