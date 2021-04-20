#!bin/sh

# default size, as in makefile, 4096^2

#  arg1: dag/omp
function test_all_parameters() 
{
    for bk1 in 4 8 16 32 64 128 256 512 1024 4094
    do
        for bk2 in 4 8 16 32 64 128 256  512 1024 4096
        do
            for nthreads in  1 2 4 8
            do
                for((i=0;i<$num_run;i++))
                do
                    echo $niter  $bk1 $bk2 $nthreads >>$logfile
                    echo -n $niter $bk1 $bk2 $nthreads >>$outfile
                    ./adifull_$1 $niter $bk1 $bk2 $nthreads >tmp
		            grep "exec time : " tmp | awk '{print $4}' >> $outfile
		            cat tmp >>$logfile
                done
            done
        done
    done
}

#$1, dag/omp
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
            ./adifull_$1 $niter  $2 $3  $nthreads >tmp
		    grep "exec time : " tmp | awk '{print $4}' >> $outfile
		    cat tmp >>$logfile
        done
    done
}
#$1, dag/omp
function get_exec()
{
    make clean
	make $1  EXTRA_FLAGS="-DN=4096 "
}

#$1, dag/omp
#$2, tile_sz1
#$3, tile_sz2
function do_one_test()
{
    get_exec  $1 
    test_one_parameter $1 $2 $3 
}


testcase=('adifull')
flagname=("")

lenth=${#testcase[*]}
num_run=5
niter=40
for((i=0;i<$lenth;i++))
do
	curdate=$(date +%Y_%m_%d_%H_%M_%S)
    outfile=${testcase[$i]}_$curdate.txt
    logfile=${testcase[$i]}_$curdate.log

#    do_one_test omp 128 128
#    do_one_test omp 256 256
    do_one_test dag 128 128 
done

