#!bin/sh
#$0,script name 

# default size, as in makefile, 4096^2

#  arg1: dag_1d/omp_1d/dag/omp
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
                    echo $niter $bk1 $bk2 $nthreads >>$logfile
                    echo -n $niter $bk1 $bk2 $nthreads >>$outfile
                    ./fdtd_$1 $bk1 $bk2 $nthreads >tmp
		            grep "exec time : " tmp | awk '{print $4}' >> $outfile
		            cat tmp >>$logfile
                done
            done
        done
    done
}

#$1, dag_1d/omp_1d/dag/omp
#$2, tile_sz1
#$3, tile_sz2
function test_best() 
{

    for nthreads in 1 2 4 8
    do
        for((i=0;i<$num_run;i++))
        do
            echo $niter $2 $3 $1 $nthreads " ">>$logfile
            echo -n $niter  $2 $3 $1 $nthreads " ">>$outfile
            ./fdtd_$1 $niter $2 $3  $nthreads >tmp
		    grep "exec time : " tmp | awk '{print $4}' >> $outfile
		    cat tmp >>$logfile
        done
    done
}
#$1, dag_1d/omp_1d/dag/omp
function get_exec()
{
    make clean
	make $1  OPT='-O3' EXTRA_FLAGS="-Dnx=4096 -Dny=4096"
}

#$1, dag_1d/omp_1d/dag/omp
#$2, tile_sz1
#$3, tile_sz2
function do_one_test()
{
    get_exec  $1 
    test_best $1 $2 $3 
}


testcase=('fdtd')
flagname=("")

lenth=${#testcase[*]}
num_run=1
niter=4
for((i=0;i<$lenth;i++))
do
	curdate=$(date +%Y_%m_%d_%H_%M_%S)
    outfile=${testcase[$i]}_$curdate.txt
    logfile=${testcase[$i]}_$curdate.log

    do_one_test omp 128 128
#    do_one_test omp 256 256
    do_one_test dag 4 4096 
done

