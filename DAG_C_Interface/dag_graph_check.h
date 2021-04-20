#ifndef _DAG_GRAPH_CHECK_H
#define _DAG_GRAPH_CHECK_H

#ifdef DEBUG_GRAPH

#include <vector>
#include <iostream>
#include <fstream>

#include "aceMesh_task.h"

namespace AceMesh_runtime {
class dag_graph_check 
{
    private:
        std::string output_filename;
        std::ofstream out;
        std::vector<std::string> loop_infos;
        int loop_id;
        int task_id;
    public:
        //dag_graph_check();
        ~dag_graph_check();
        void init(const std::string& out_dir);
		void add_loop_info(const std::string& loop_name);
        std::string  get_last_loop_info();
        void set_task_loop_id(aceMesh_task* t);
        bool bfs(std::vector<aceMesh_task*>& start_tasks);
        void print_loop_depedence(std::vector<aceMesh_task*>& start_tasks, int task_nums);
        void print_id_loop_info_table();
        bool check_graph_dfs(std::vector<aceMesh_task*>& start_tasks, int task_nums);
        bool check_graph_bfs(std::vector<aceMesh_task*>& start_tasks, int task_nums, bool print_info = false);
        void dfs(aceMesh_task* root, int& task_nums, int deep_length);
        void clear();
        void print_stack();
};
}
#endif
#endif
