#!bin/sh

#  arg1: dag/omp
function test_all_parameters() 
{
    for nbk in 64 128 256 512 
    do
        for dimext in 64 128 256 512
        do
            for nthreads in  1 2 4 8
            do
                for((i=0;i<$num_run;i++))
                do
                    echo $nbk $dimext $nthreads >>$logfile
                    echo -n $nbk $dimext $nthreads >>$outfile
                    ./cholesky_$1 $nbk $dimext $nthreads >tmp
		            grep "exec time : " tmp | awk '{print $4}' >> $outfile
		            cat tmp >>$logfile
                done
            done
        done
    done
}

#$1, dag/omp
#$2, nblocks
#$3, dimExt
function test_best() 
{

    for nthreads in 1 2 4 8
    do
        for((i=0;i<$num_run;i++))
        do
            echo $2 $3 $1 $nthreads " ">>$logfile
            echo -n $2 $3 $1 $nthreads " ">>$outfile
            ./cholesky_$1 $2 $3  $nthreads >tmp
		    grep "exec time : " tmp | awk '{print $4}' >> $outfile
		    cat tmp >>$logfile
        done
    done
}
#$1, dag/omp
function get_exec()
{
    make clean
	make $1  
}

#$1, dag/omp
#$2, nblocks
#$3, dimExt
function do_one_test()
{
    get_exec  $1 
    test_best $1 $2 $3 
}


testcase=('cholesky')
flagname=("")

lenth=${#testcase[*]}
num_run=5
export LD_LIBRARY_PATH=/opt/OpenBLAS/lib:$LD_LIBRARY_PATH
for((i=0;i<$lenth;i++))
do
	curdate=$(date +%Y_%m_%d_%H_%M_%S)
    outfile=${testcase[$i]}_$curdate.txt
    logfile=${testcase[$i]}_$curdate.log

    do_one_test omp 70 70
    do_one_test dag 70 70 
done

