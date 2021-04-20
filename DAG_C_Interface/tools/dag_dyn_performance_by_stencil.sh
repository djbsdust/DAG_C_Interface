fs=$1
fe=$2
path_file=""
for((t=$fs;t<=$fe;t++));do
  path_file=$path_file" ./$t/scheduler_thread_dyn_*.csv "
done
cat $path_file | awk -F','  '{if (NF>4) { \
                   name=$1;\
                   gsub(/ /,"", name);\
                   #accumulate the times of being called
                   a[name]++; \
                   
                   #accumulate exec time for this task
                   time = $2;\
                   time = time / 1000000000;\
                   b[name]+= time; \

                   #accumulate exec time for all the tasks
                   total_exec_time += time;\

                   #the number of cache miss
                   c[name]+=$3;\
                   
                   #accumulate cache miss for all the tasks
                   total_cache_miss += $3;\

                   # the number of vert_scheduled
                   d[name]+=$(NF-1);\

                   #accumulate vertical scheduled for all the tasks
                   total_vert+=$(NF-1);\
               
                   #the number of stolen 
                   e[name]+=($NF);\

                   #accumulate stolen times for all the tasks
                   total_stolen+=($NF);\

                   total_task++;\
                 }} \
                 END {\
                   print "name,called,exec,exec_prop,miss,miss_prop,total_vsch,vsch_prop,total_stolen,stolen_prop";\
                   for (i in a) {\
                     if (total_exec_time==0){total_exec_time=1;}\
                     if (total_cache_miss==0){total_cache_miss=1;}\
                     if (total_vert==0){total_vert=1;}\
                     if (total_stolen==0){total_stolen=1;}\
                     if (total_task==0){total_task=1;}\
                     if (a[i]==0){a[i]=1;}\
                     print i","a[i] \
                             ","b[i] \
                             ","b[i] / total_exec_time \
                             ","c[i] \
                             ","c[i] / total_cache_miss \
                             ","d[i] \
                             ","d[i] / a[i] \
                             ","e[i] \
                             ","e[i] / a[i]; \
                   }\
                   print "total task," total_task;\
                   print "total exec time," total_exec_time;\
                   print "total cache miss," total_cache_miss;\
                   print "total vertical scheduled," total_vert;\
                   #print "total vertical scheduled prop," total_vert/total_task;\
                   print "total stolen," total_stolen;}'
