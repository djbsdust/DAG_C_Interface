fs=$1
fe=$2
path_file=""
for((t=$fs;t<=$fe;t++));do
  path_file=$path_file" ./$t/scheduler_thread_*.csv "
done
cat $path_file | awk -F','  '{if (NF>9) { \
                   name=$NF;\
                   gsub(/ /,"", name);\
                   # the number of if being good vertical task
                   a[name]+=$(NF-1);\
                   # the number of the same addr as the vertical task
                   b[name]+=$(NF-2);\
                   # the number of the same addr among the successors of this task
                   c[name]+=$(NF-3);\
                   # the nuber of the successors of this task
                   d[name]+=$(NF-4);\
                   #the number of neighbour addrs
                   e[name]+=$(NF-5);\
                   #the number of non-neighbour addrs
                   f[name]+=$(NF-6);\
                   #accumulate the times of being called
                   g[name]++; \
                   total_task++;\
                 }} \
                 END {\
                   print "name,called,total_goodv,goodv_avg,total_vaddr,vaddr_avg,total_saddr,saddr_avg,total_succ,succ_avg,total_neigh,neigh_avg,total_non-neigh,non-neigh_avg";\
                   for (i in a) {\
                     if (d[i]==0){d[i]=1;}\
                     if (g[i]==0){g[i]=1;}\
                     print i","g[i]  \
                             ","a[i] \
                             ","a[i] / g[i] \
                             ","b[i] \
                             ","b[i] / g[i] \
                             ","c[i] \
                             ","c[i] / g[i] / d[i] \
                             ","d[i] \
                             ","d[i] / g[i] \
                             ","e[i] \
                             ","e[i] / g[i] \
                             ","f[i] \
                             ","f[i] / g[i]; \
                   }\
                   print "total task," total_task;}'
