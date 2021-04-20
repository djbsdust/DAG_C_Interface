result=result.log
for((i=1;i<20;i++))
do
    make dag >> $result
    echo "bsub -I -q q_x86_expr -n 1 ./3d7p_dag 10 16 16 8" >> $result
    bsub -I -q q_x86_expr -n 1 ./3d7p_dag 10 16 16 8 >> $result
    
    make dag >> $result 
    echo "bsub -I -q q_x86_share -n 1 ./3d7p_dag 10 16 16 8" >> $result
    bsub -I -q q_x86_share -n 1 ./3d7p_dag 10 16 16 8 >> $result
done
grep "exec time" $result
