#!/usr/bin/env python
import sys

##########################################################
# Assumptions:
#   1) last field of csv file is time stamp.
#   2) thread_num <256
#
# Usage: 
#    python merge_trace.py num_threads
##########################################################

def read_line_from_file(a_file):
    return a_file.readline()

def get_time_from_line(str):
    my_list = str.split(',')
    return my_list[-1]

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print 'input arg error'
        exit(0)
    thread_num = int(sys.argv[1])
    if thread_num <= 0 or thread_num >=256 :
        print "input error"
        exit(0)
    file_list = []
    for t in range(0, thread_num):
        file_list.append( open("scheduler_thread_"+str(t)+".csv", 'r') )


    trace_out = open("total_trace.csv", 'w')
    while True:
        #read one task graph
        #TODO, opt, use win or lose tree. A loser write this code
        data_list = []
        time_list = []
        flag = False
        for i in range(0, thread_num):
            this_str = read_line_from_file(file_list[i])
            if not this_str:
                flag = True
                break
            data_list.append( this_str )
            time_list.append( int(get_time_from_line( this_str ) ) )
        if flag is True:
            break

        while True:
            min_index = -1
            for i in range(0, thread_num):
                if time_list[i] == -1:
                    continue
                else:
                    min_index = i
                    break
            if min_index == -1:
                break

            for i in range(min_index + 1, thread_num):
                if time_list[i] == -1:
                    continue
                if time_list[i] < time_list[min_index]:
                    min_index = i

            if min_index == -1:
                break

            trace_out.write(data_list[min_index])
            #update
            this_line = read_line_from_file(file_list[min_index])
            if this_line == "spawn\n":
                time_list[min_index] = -1
            else:
                data_list[min_index] = this_line
                time_list[min_index] = get_time_from_line(this_line)
        trace_out.write("spawn\n")

    #close all file
    for i in range(0, thread_num):
        file_list[i].close()
    trace_out.close()
