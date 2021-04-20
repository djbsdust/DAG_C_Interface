#!/usr/bin/env python
class dataItem:
    def __init__(self):
        self.processor = 0
        self.physical_id = 0
        self.core_id = 0
    def get_name(self,line):
        mylist = line.split(':')
        return mylist[0].strip()

    def get_value(self,line):
        mylist = line.split(':')
        return mylist[1].strip()

    def read_item(self,f):
        while True:
            tmp = f.readline()
            if not tmp:
                return False
            if "processor" in self.get_name(tmp):
                self.processor = int(self.get_value(tmp))
            if "physical id" in self.get_name(tmp):
                self.physical_id = int(self.get_value(tmp))
            if "core id" in self.get_name(tmp):
                self.core_id = int(self.get_value(tmp))
                #print self.processor,self.physical_id,self.core_id
                return True

        return True

    def __cmp__(self,other):
        if isinstance(other, dataItem):
            if self.physical_id < other.physical_id:
                return True
            if self.core_id < other.core_id:
                return True
            if self.processor < other.processor:
                return True
            return True
        else:
            return -1

if __name__ == "__main__":
    f = file('/proc/cpuinfo','r')
    line = "aaa"
    proc_list = []
    while True:
        d = dataItem()
        if d.read_item(f) is False:
            break
        proc_list.append(d)
        #print line
    #proc_list.sort()
    phy_to_processor = {}
    for itr in proc_list:
        if( phy_to_processor.has_key(itr.physical_id) ):
            if phy_to_processor[itr.physical_id].has_key(itr.core_id):
                phy_to_processor[itr.physical_id][itr.core_id].append(itr.processor)
            else:
                mylist = []
                mylist.append(itr.processor)
                phy_to_processor[itr.physical_id][itr.core_id] = mylist
        else:
            tmp_list = []
            tmp_list.append(itr.processor)
            phy_to_processor[itr.physical_id] = {}
            phy_to_processor[itr.physical_id][itr.core_id] = tmp_list

    for  phy_id in phy_to_processor.keys():
        print "phy_id: ", phy_id, "include :"
        for core_id in phy_to_processor[phy_id].keys():
            print "\tcore_id: ", core_id, "include :"
            phy_to_processor[phy_id][core_id].sort()
            for processor_id in phy_to_processor[phy_id][core_id]:
                print  "\t\t", "processor ", processor_id

