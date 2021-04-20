#!/usr/bin/env python
import sys
import os

#####################################################################
#  A tool for compute task reuse distance    
#  Usage: 
#    python reuse_distance_merge.py thread_num [merge]
#  
#  Assumptions:
#    please refer to reuse_distance.py
#  
#  Mechanism:
#  First, merge all task trace files (scheduler_thread_#.csv) into 
#  scheduler_thread_merge.csv file. And sort task info by timestamp 
#  just before the task execute. Then use the merged file to compute 
#  task reuse distance. 
#  FIXME: All tasks with the same task_id are considered to have the 
#  same data addr.
#####################################################################

def read_line_from_file(a_file):
    return a_file.readline()

def get_time_from_line(str):
    my_list = str.split(',')
    return my_list[-1] #last col

def merge_file(thread_num):
    file_list = []
    for t in range(0, thread_num):
        file_list.append( open("scheduler_thread_"+str(t)+".csv", 'r') )


    trace_out = open("scheduler_thread_merge.csv", 'w')

    data_list = []
    time_list = []

    #Read first line of each files
    for i in range(0, thread_num):
        this_str = read_line_from_file(file_list[i])
        data_list.append( this_str )
        time_list.append( int(get_time_from_line(this_str)) )

    linenum = 0
    while True:
        #find min (time_list) : min timestemp
        min_index = -2
        for j in range(0, thread_num):
            if time_list[j] != -1:
                min_index = j
                break;

        if min_index == -2 :
            print "Done! Total:", linenum
            break;
        
        #print time_list
        for i in range(j+1, thread_num):
            if time_list[i] == -1:
                continue
            if time_list[i] < time_list[min_index]:
                    min_index = i 
        #print min_index
        #print time_list[min_index]
        
        #if time_list[min_index] == -1:
        #    break

        trace_out.write(str(min_index)+", "+data_list[min_index])
        linenum = linenum + 1
        this_line = read_line_from_file(file_list[min_index])
        if this_line == "spawn\n":
            time_list[min_index] = -1
        else:
            data_list[min_index] = this_line
            time_list[min_index] = int( get_time_from_line(this_line) )
    
    trace_out.write("spawn\n")

    #close all file
    for i in range(0, thread_num):
        file_list[i].close()
    trace_out.close()

    #os.system("rm -f scheduler_thread_*.csv")

def get_origin_data(input_file):
    res = []
    while True:
        line = input_file.readline()
        #print line
        if not line:
            break
        if line is None:
            continue
        if line == "spawn\n":
            break
        line = line.strip('\n')

        #loop_id:1 task_id:3 task_type 5
        origin_data = line.split(',')

        #if task_type is not STENCIL
        if int(origin_data[6]) != 1: 
            continue
        
        #(loop_id, task_id, task_type)
        res.append((int(origin_data[2]), int(origin_data[4]), int(origin_data[6])))
        #print res
    return res

def get_resue_distance(trace_list):
    #output = "reuse_info.log"
    #f = open(output, 'w')

    # dict: distance[task_id] = task_counter
    distance = {}
    curlen = 0
    task_counter = 0
    valid_task_counter = 0

    sum = 0;
    for item in trace_list:
        #print item 
        #item : (loop_id, task_id, task_type)
        if distance.has_key(item[1]):
            tmp = []
            j = task_counter-1
            while j >= 0 and trace_list[j][1] != item[1]:
                tmp.append(trace_list[j][1])
                j = j -1
            s = set(tmp)
            #print s
            curlen = len(s)
            valid_task_counter = valid_task_counter + 1
            #f.write(str(len(s)) + " " + str(task_counter) + " " + str(valid_task_counter) + " " + str(item[1]) + "\n")
            distance[item[1]] = task_counter
            sum = sum + len(s)   
        else:
            #f.write(str(curlen) + " " + str(task_counter) + " " + str(valid_task_counter) + "\n")
            distance[item[1]] = task_counter
        task_counter = task_counter + 1
    
    #print (task_counter - valid_task_counter)
    return float(sum)/(valid_task_counter)

def reuse_distance_merge(thread_num):
    input_file = open("scheduler_thread_merge.csv", "r")
    average_reuse_distance = 0
    #extract (loop_id, task_id, task_type)
    data_list = get_origin_data(input_file)

    average_reuse_distance = get_resue_distance(data_list)
    print "average reuse distance :", average_reuse_distance

    input_file.close()    

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "args: thread_num [merge]"
        exit(0)

    thread_num = int(sys.argv[1])
    if thread_num <= 0 or thread_num >=256 :
        print "input error"
        exit(0)

    mergeonly = False
    if len(sys.argv) == 3 :
        flag = sys.argv[2]
        if flag == 'merge' :
            mergeonly = True          
        else :
            print "args warning: only support merge option"

    merge_file(thread_num)
    if mergeonly:
        exit(0)

    reuse_distance_merge(thread_num)

