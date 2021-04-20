#!bin/bash

# default size, as in makefile, 256^3

#  arg1: cdag/omp/fdag
function test_all_parameters() 
{
    for bk1 in 4 8 16 32 64 128 256 
    do
        for bk2 in 4 8 16 32 64 128 256
        do
            for nthreads in  1 2 4 8
            do
                for((i=0;i<$num_run;i++))
                do
                    echo $niter $bk1 $bk2 $nthreads >>$logfile
                    echo -n $niter $bk1 $bk2 $nthreads >>$outfile
                    ./3d7p_$1 $niter $bk1 $bk2 $nthreads >tmp
		            grep "exec time : " tmp | awk '{print $4}' >> $outfile
		            cat tmp >>$logfile
                done
            done
        done
    done
}

#$1, cdag/omp/fdag
#$2, tile_sz1
#$3, tile_sz2
function test_one_parameter() 
{
    for nthreads in 1 2 4 8
    do
        for((i=0;i<$num_run;i++))
        do
            echo $niter $2 $3 $1 $nthreads " ">>$logfile
            echo -n $niter $2 $3 $1 $nthreads " ">>$outfile
            ./3d7p_$1 $niter $2 $3  $nthreads >tmp
		    grep "exec time : " tmp | awk '{print $4}' >> $outfile
		    cat tmp >>$logfile
        done
    done
}
#$1, cdag/omp/fdag
function get_exec()
{
    make clean
	make $1 
}

#$1, cdag/omp/fdag
#$2, tile_sz1
#$3, tile_sz2
function do_one_test()
{
    get_exec  $1 
    test_one_parameter $1 $2 $3 
}


testcase=('3d7p')
flagname=("")

lenth=${#testcase[*]}
num_run=5
for((i=0;i<$lenth;i++))
do
	curdate=$(date +%Y_%m_%d_%H_%M_%S)
    outfile=${testcase[$i]}_$curdate.txt
    logfile=${testcase[$i]}_$curdate.log
    niter=40
    do_one_test omp 8 16
    do_one_test omp 16 16
    do_one_test cdag 16 16 
done

export LIBRARY_PATH=/home/lyt/git/tbb/build/linux_intel64_gcc_cc8.3.0_libc2.28_kernel4.19.0_release
