#!bin/sh
 
function test_all_parameters()
{
	xbs=8
	ybs=8
	iter=50
	nthreads=28
	for((i=0;i<$num_run;i++))	
		do
			echo $iter $xbs $ybs $nthreads >>$logfile
			echo $iter $xbs $ybs $nthreads >>$outfile
			./3d7p_dag $iter $xbs $ybs $nthreads >tmp
			grep "exec time : " tmp | awk '{print $4}' >> $outfile
			grep "total time: " tmp | awk '{print $3}' >> $outfile
			cat tmp >>$logfile
		done
}


testcase=('3d7p')

lenth=${#testcase[*]}
num_run=5
for((i=0;i<$lenth;i++))
	do
		curdate=$(date +%Y_%m_%d_%H_%M_%S)
		outfile=${testcase[$i]}_$curdate.txt
		logfile=${testcase[$i]}_$curdate.log

		test_all_parameters
	done

