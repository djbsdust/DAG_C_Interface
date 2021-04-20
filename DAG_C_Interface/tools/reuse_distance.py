#!/usr/bin/env python
import sys
############################################################################ 
# Assupmtions:
#  1) apps should specify each task's task type, we care about STENCIL_TASK
#  2) in csv files, the ordering of the first 6 fields cannot be changed, 
#      since this tool will checked the 4th(task_id) and 6th field(task_type
#      , to see if it is stencil task or critical task).
#  3) task_id's solo appearance is not counted in reuse degree computation.
#
# Usage and mechanesm:
#   similar to reuse_distance_merge.py
###########################################################################

def get_resue_distance(trace_list, output):
    distance = {}
    #f = open(output, 'w')
    i = 0;

    sum = 0;
    for item in trace_list:
        #print item
        if distance.has_key(item[1]):
            tmp = []
            j = i-1
            while j >= 0 and trace_list[j][1] != item[1]:
                tmp.append(trace_list[j][1])
                j = j -1
            s = set(tmp)
            #print>>f,s
            #f.write(s);
            #f.write(str( len(s) )  + "\n")
            distance[item[1]] = i
            sum = sum + len(s)
        else:
            #f.write("-1\n")
            distance[item[1]] = i
        i = i + 1
    #print "task num: "+str(len(distance)) 
    #print "total loop: "+str(i)
    return float(sum)/(i)

def get_list(input_file):
    res = []
    while True:
        line = input_file.readline()
        if not line:
            break
        if line is None:
            continue
        if line == "spawn":
            break
        line = line.strip('\n')
        my_list = line.split(',')
        if(len(my_list) < 5):
            continue
        if int(my_list[5]) != 1:
            continue
        res.append((int(my_list[1]), int(my_list[3]), int(my_list[5])))
    return res

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print 'input arg error'
        exit(0)

    thread_num = int(sys.argv[1])
    if thread_num <= 0 or thread_num >=256 :
        print "input error"
        exit(0)
    input_list = []
    for i in range(0,thread_num):
        input_file = open("scheduler_thread_"+str(i)+".csv", 'r')
        input_list.append(input_file)
    #input_list = ["scheduler_thread_0.csv"]
    while True:
        i = 0;
        total_reuse_distance = 0
        for item in input_list:
            my_list = get_list(item)
            #print my_list
            output = "reuse_distance_"+str(i)+".csv"
            i = i + 1
            total_reuse_distance = total_reuse_distance + get_resue_distance(my_list, output)
        print "average resue distance : ", total_reuse_distance/i
        choice = raw_input("have completed an exe,Continue ? (y/n): ")
        if choice == "y" and i<thread_num:
            continue
        else:
            break
