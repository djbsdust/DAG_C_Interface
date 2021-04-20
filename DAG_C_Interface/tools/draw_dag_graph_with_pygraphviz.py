#!/usr/bin/env python
import sys
import subprocess
import os
import pygraphviz as pgv

class pgv_output:
    def __init__(self):
        self.obj_id = 0
        self.output_name = ""
        self.A=pgv.AGraph(directed=True,strict=True)
        self.dict_obj = {}

    def set_output(self,output_name):
        self.output_name = output_name
        self.A=pgv.AGraph(directed=True,strict=True)

    def inc_obj_id(self):
        res = self.obj_id
        self.obj_id = self.obj_id + 1
        return res

    def get_obj_id(self, loop_id, task_id):
        if self.dict_obj.has_key(loop_id):
            if self.dict_obj[loop_id].has_key(task_id):
                return self.dict_obj[loop_id][task_id]
            else:
                self.dict_obj[loop_id][task_id] = self.inc_obj_id()
                return self.dict_obj[loop_id][task_id]
        else:
            self.dict_obj[loop_id] = {}
            self.dict_obj[loop_id][task_id] = self.inc_obj_id()
            return self.dict_obj[loop_id][task_id]


    def add_node(self, obj_id, name):
        self.A.add_node(obj_id)
        n = self.A.get_node(obj_id)
        n.attr['label'] = name
        return
    def add_color_node(self, obj_id, name, node_color):
        self.A.add_node(obj_id)
        n = self.A.get_node(obj_id)
        n.attr['label'] = name
        n.attr['fillcolor'] = node_color
        n.attr['style'] = 'filled'
        return

    def add_loop_edge(self, from_loop_id, from_loop_label, to_loop_id, to_loop_label, is_v):
        self.add_node(from_loop_id, from_loop_label)
        self.add_node(to_loop_id, to_loop_label)
        if is_v is True:
            self.A.add_edge(from_loop_id, to_loop_id, color = 'red')
        else :
            self.A.add_edge(from_loop_id, to_loop_id, color = 'black')


    def add_edge(self, from_loop_id, from_task_id, to_loop_id, to_task_id, is_v, from_node_color, to_node_color):
        from_obj = self.get_obj_id(from_loop_id, from_task_id)
        to_obj = self.get_obj_id(to_loop_id, to_task_id)

        self.add_color_node(from_obj, str(from_loop_id) + ":" + str(from_task_id), from_node_color)
        self.add_color_node(to_obj, str(to_loop_id) + ":" + str(to_task_id), to_node_color)
        if is_v is True:
            self.A.add_edge(from_obj, to_obj, color = 'red')
        else:
            self.A.add_edge(from_obj, to_obj, color = 'black')

    def show(self):
        self.A.graph_attr['epsilon']='0.001'
        #print self.A.string() # print dot file to standard output
        #self.A.write('foo.dot')
        self.A.layout('dot') # layout with dot
        #self.A.layout('sfdp') # layout with dot
        #twopi
        self.A.draw(self.output_name) # write to file


class graph:
    def __init__(self, tasks_data):
        self.task_threads =  tasks_data[0]
        self.task_list =  tasks_data[1]
        self.loop_node_dict = {}
        self.edge_dict = {}
        self.loop_dep = {}
        self.output = pgv_output()
        self.color_list = ('#FFFFFF','#FFFF00','#B0E0E6','#FF6100','#40E0D0','#FF0000','#2E8B57','#0000FF',
               '#F5DEB3', '#32CD32',  '#802A2A', '#734A12', '#5E2612', '#6A5ACD', '#9933FA', '#A0522D'  )

    def reset(self, tasks_data):
        self.task_threads =  tasks_data[0]
        self.task_list =  tasks_data[1]
        self.loop_node_dict = {}
        self.edge_dict = {}
        self.loop_dep = {}

    def get_color(self, loop_id, task_id):
        return self.color_list[self.task_threads[loop_id][task_id]]

    def add_node(self, node_str):
        mylist = node_str.split(':')
        loop_id = int(mylist[0].strip())
        name = mylist[1].strip()
        self.loop_node_dict[loop_id] = name
        self.edge_dict[loop_id] =  {}
        self.loop_dep[loop_id] = {}

    def add_edge(self, edge_str):
        tmplist = edge_str.split(',')
        if len(tmplist) < 6:
            return
        from_loop_id = int(tmplist[1])
        from_task_id = int(tmplist[3])


        length = len(tmplist)
        mylist = []
        for i in range(4,length, 4):
            is_v = False
            if 'v_child' in tmplist[i] :
                is_v = True
            to_loop_id = int(tmplist[i+1])
            to_task_id = int(tmplist[i+3])
            if self.loop_dep[from_loop_id].has_key(to_loop_id)  :
                self.loop_dep[from_loop_id][to_loop_id] = (is_v or self.loop_dep[from_loop_id][to_loop_id])
            else :
                self.loop_dep[from_loop_id][to_loop_id]  = is_v
            mylist.append(  (to_loop_id, to_task_id, is_v) )
        self.edge_dict[from_loop_id][from_task_id] = mylist

    def is_v_task(self, from_loop_id, from_task_id, loop_id2, task_id2):
        if from_task_id == -1 :
            return False

        if not self.edge_dict[from_loop_id].has_key(from_task_id) :        
            return False

        if len( self.edge_dict[from_loop_id]) == 0 or len( self.edge_dict[from_loop_id][from_task_id] )  == 0:
            return False

        for tup in self.edge_dict[from_loop_id][from_task_id] :
            if tup[0] == loop_id2 and tup[1] == task_id2 and (tup[2] is True):
                return True
        return False


    def statistics_graph(self):
        total_edges = 0
        v_edges = 0
        for loop_id in self.edge_dict:
            for task_id in self.edge_dict[loop_id]:
                total_edges = total_edges + len(self.edge_dict[loop_id][task_id])
                for tup in self.edge_dict[loop_id][task_id] :
                    if tup[2] == True:
                        v_edges = v_edges + 1
        print "total edges: ", total_edges, "vertical edges: ", v_edges
        reuse_time = {}
        for per_thread in self.task_list:
            last_loop_id = -1
            last_task_id = -1
            tmp_reuse_times = 0
            print "thread ", per_thread, " have done task_num: ", len(self.task_list[per_thread])
            for tup in self.task_list[per_thread]:
                if self.is_v_task(last_loop_id, last_task_id, tup[0], tup[1])  is True:
                    tmp_reuse_times = tmp_reuse_times + 1
                else:
                    if reuse_time.has_key(tmp_reuse_times):
                        reuse_time[tmp_reuse_times] = reuse_time[tmp_reuse_times] + 1
                    else :
                        reuse_time[tmp_reuse_times] =  1
                    tmp_reuse_times = 0
                last_loop_id = tup[0]
                last_task_id = tup[1]
        print reuse_time

    def print_loop_graph(self, filename):
        self.output.set_output(filename)
        for loop_id in self.loop_node_dict:
            #if len(self.loop_dep[loop_id].keys()) > 0:
                #print "\t have relationship with ",
            for to_loop_id in self.loop_dep[loop_id].keys():
                self.output.add_loop_edge(loop_id,  str(loop_id) + ':' + self.loop_node_dict[loop_id] , to_loop_id, str(to_loop_id) + ':' + self.loop_node_dict[to_loop_id], self.loop_dep[loop_id][to_loop_id])
                #print "loop id:", to_loop_id,
            #if len(self.loop_dep[loop_id].keys()) > 0:
                #print "\n"
        self.output.show()

    def print_graph(self, filename):
        self.output.set_output(filename)
        for loop_id in self.edge_dict:
            for task_id in self.edge_dict[loop_id]:
                for data_tuple in self.edge_dict[loop_id][task_id]:
                    self.output.add_edge(loop_id, task_id, data_tuple[0], data_tuple[1], data_tuple[2], self.get_color(loop_id, task_id), self.get_color(data_tuple[0], data_tuple[1]))
        self.output.show()

    def print_relationship(self, id1, id2):
        if  (self.edge_dict.has_key(id1)  and self.edge_dict.has_key(id2) ) is False:
            print 'input error'
            return
        self.output.set_output('two_loop_graph.png')
        for task_id in self.edge_dict[id1]:
            for data_tuple in self.edge_dict[id1][task_id]:
                if data_tuple[0] == id2:
                    self.output.add_edge(id1, task_id, data_tuple[0], data_tuple[1], data_tuple[2], self.get_color(id1, task_id), self.get_color(data_tuple[0], data_tuple[1]))

        self.output.show()
        return

def judge_node(in_str):
    mylist = in_str.split(":")
    if len(mylist) > 1:
        return True;
    return False;

def init_read_file(thread_num):
    thread_order = []
    for i in range(0, thread_num):
        thread_order.append( open('scheduler_thread_' + str(i) + '.csv','r'))
    return  thread_order

def uninit_read_file(thread_order):
    for item in thread_order:
        item.close()

def read_one_order(thread_order):
    task_thread_dict = {}
    task_list = {}
    i = 0
    #read exec order from file
    for item in thread_order:
        task_list[i] = []
        while  True:
            line = item.readline()
            if not line:
                break
            if line is None:
                continue
            if 'spawn' in line:
                break
            mylist = line.split(',')
            loop_id = int(mylist[1])
            task_id = int(mylist[3])
            task_list[i].append( (loop_id, task_id) )
            if task_thread_dict.has_key(loop_id):
                task_thread_dict[loop_id][task_id] = i
            else:
                task_thread_dict[loop_id] = {}
                task_thread_dict[loop_id][task_id] = i
        i = i + 1
    #print task_thread_dict
    return  (task_thread_dict, task_list)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print 'input arg error'
        exit(0)

    thread_num = int(sys.argv[1])
    if thread_num > 16 :
        print "color now only support 16"
        exit(0)
    task_list = {}
    thread_order = init_read_file(thread_num)

    g = graph(read_one_order(thread_order))
    #read from file
    input = open("loop_dependence.info", 'r')
    loop_graph_file="loop_graph"
    total_graph_file="total_graph"
    spawn_num=0
    while True:
        line = input.readline()
        if not line:
            break
        if line is None:
            continue

        line = line.strip()
        if line == '#':
            lname=loop_graph_file+str(spawn_num)+".png"
            g.print_loop_graph(lname)
            tname=total_graph_file+str(spawn_num)+".jpg"
            g.print_graph(tname)
            g.statistics_graph()
            choice = raw_input("have completed an exe,Continue ? (y/n): ")
            if choice == "y":
                spawn_num+=1
                g.reset(read_one_order(thread_order))
                continue
            else:
                print "break at the this #"
                break
        if judge_node(line):
            g.add_node(line)
        else:
            g.add_edge(line)

    #g.print_loop_info()
    #id1 = int( raw_input("please input the fisrt loop id: ") )
    #id2 = int( raw_input("please input another loop id: ") )

    #g.print_relationship(id1, id2)
    uninit_read_file(thread_order)

