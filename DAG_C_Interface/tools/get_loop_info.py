#!/usr/bin/env python
import subprocess

class graph:
    def __init__(self):
        self.loop_node_dict = {}
        self.edge_dict = {}
        self.loop_dep = {}

    def reset(self):
        self.loop_node_dict = {}
        self.edge_dict = {}
        self.loop_dep = {}

    def add_node(self, node_str):
        mylist = node_str.split(':')
        id = int(mylist[0].strip())
        name = mylist[1].strip()
        self.loop_node_dict[id] = name
        self.edge_dict[id] =  {}
        self.loop_dep[id] = []

    def add_edge(self, edge_str):
        tmplist = edge_str.split(',')
        if len(tmplist) < 4:
            return
        from_loop_id = int(tmplist[1])
        from_task_id = int(tmplist[3])
        length = len(tmplist)
        mylist = []
        for i in range(4,length, 4):
            to_loop_id = int(tmplist[i+1])
            to_task_id = int(tmplist[i+3])
            self.loop_dep[from_loop_id].append(to_loop_id)
            mylist.append(  (to_loop_id, to_task_id) )
        self.edge_dict[from_loop_id][from_task_id] = mylist

    def print_loop_info(self):
        gnu_script = open('loop_graph.gnu','w')

        gnu_script.write('set term pdfcairo lw 2 font "Times New Roman,8"\n')
        gnu_script.write('set output "loop_graph.pdf"\n')
        str_range = 'set xrange [0:%d]; set yrange [0:%d]\n'%(len(self.loop_node_dict) * 10, 20)
        gnu_script.write(str_range)
        #gnu_script.write('set object 5 circle at 5,-5 size 3 arc [0:360] fc rgb "orange" fs solid')
        #line
        objid = 1
        for id in self.loop_node_dict:
            x = id * 10
            y = 10
            str_node = 'set object %d circle at %d, %d size 3 arc [0:360] fc rgb "orange" fs solid\n'%(objid, x,y)
            str_node_name =  'set label %s at %d,%d'%(self.loop_node_dict[id],x,y)
            objid=objid+1
            gnu_script.write(str_node)
            gnu_script.write(str_node_name)
            print "id = ", id , "loop name = ", self.loop_node_dict[id]
            loops = set( self.loop_dep[id])
            loops = sorted(loops)
            if len(loops) > 0:
                print "\t have relationship with ",
            for loop_id in loops:
                print "loop id:", loop_id,
                mid = (x + loop_id * 10 )/2
                str_line = 'set object %d circle at %d, %d size %d arc [0:180] fc rgb "orange"\n'%(objid, mid, y, mid - x)
                objid=objid+1
                #str_line = "set arrow from %d, %d to %d, %d\n"%(x, y, mid, y + 5 )
                #str_line += "set arrow from %d, %d to %d, %d\n"%(mid, y+5, loop_id * 10, y)
                gnu_script.write(str_line)
            if len(loops) > 0:
                print "\n"
        gnu_script.write("plot -10 notitle\nset output\n")
        gnu_script.write("quit")

    def print_relationship(self, id1, id2):
        objid = 1
        gnu_script = open('two_loop_task_graph.gnu','w')

        gnu_script.write('set term pdfcairo lw 2 font "Times New Roman,8"\n')
        gnu_script.write('set output "two_loop_task_graph.pdf"\n')

        task_nums  =  len(self.edge_dict[id1])
        for i in range(0,task_nums):
            str_node = 'set object %d circle at %d, %d size 1 arc [0:360] fc rgb "orange" fs solid\n'%(objid, i * 10 , 15)
            str_node_name =  'set label %d at %d,%d'%( self.edge_dict[id1], i*10, 15)
            objid=objid+1
            gnu_script.write(str_node)
            gnu_script.write(str_node_name)

        if task_nums < len(self.edge_dict[id2]) :
            task_nums = len(self.edge_dict[id2])
        for i in range(0,task_nums):
            str_node = 'set object %d circle at %d, %d size 1 arc [0:360] fc rgb "orange" fs solid\n'%(objid, i * 10 , 10)
            str_node_name =  'set label %d at %d,%d'%( self.edge_dict[id2], i*10, 10)
            objid=objid+1
            gnu_script.write(str_node)
            gnu_script.write(str_node_name)

        for task_id in self.edge_dict[id1]:
            objid = objid + 1
            print "loop_id = ",id1,"  loop_info = ", self.loop_node_dict[id1], "task id = ", task_id, "have child:"
            for t in self.edge_dict[id1][task_id]:
                if t[0] == id2:
                    print "\t", "loop id = ", id2, " loop_info = ", self.loop_node_dict[id2], "task_id = ", t[1]
                    str_line = "set arrow from %d, %d to %d, %d\n"%( task_id*10, 15 , t[1] * 10, 10)
                    gnu_script.write(str_line)

        str_range = 'set xrange [-5:%d]; set yrange [0:%d]\n'%( task_nums * 10, 20)
        gnu_script.write(str_range)
        gnu_script.write("plot -10 notitle\nset output\n")
        gnu_script.write("quit")

def judge_node(in_str):
    mylist = in_str.split(":")
    if len(mylist) > 1:
        return True;
    return False;

#def output_loop_graph():
#    proc = subprocess.Popen(['gnuplot','-p'],
#        shell=True,
#        stdin=subprocess.PIPE,
#        )
#    proc.stdin.write('set term pdfcairo lw 2 font "Times New Roman,8"\n')
#    proc.stdin.write('set output "precipitation.pdf"\n')
#    #set object 5 circle at 5,-5 size 3 arc [0:360] fc rgb "orange" fs solid
#    #line
#    proc.stdin.write('set xrange [0:10]; set yrange [-2:2]\n')
#    proc.stdin.write('plot sin(x)\nset output\n')
#    proc.stdin.write('quit\n') #close the gnuplot window

if __name__ == "__main__":
    #read from file
    input = open("loop_dependence.info", 'r')
    g = graph()
    while True:
        line = input.readline()
        if not line:
            break
        if line is None:
            continue

        line = line.strip()
        if line == '#':
            g.print_loop_info()
            choice = raw_input("have completed an exe,Continue ? (y/n): ")
            if choice == "y":
                g.reset()
                continue
            else:
                print "break at the this #"
                break
        if judge_node(line):
            g.add_node(line)
        else:
            g.add_edge(line)

    #g.print_loop_info()
    id1 = int( raw_input("please input the fisrt loop id: ") )
    id2 = int( raw_input("please input another loop id: ") )

    g.print_relationship(id1, id2)

