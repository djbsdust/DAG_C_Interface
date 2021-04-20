#!bin/sh

MIC=FALSE
TEST_HOME=/home/lchen/tsl/install/DAG_C_Interface

#$1, cdag/omp/fdag
#$2, tile_sz1
#$3, tile_sz2
function test_one_parameter() 
{
    for nthreads in 1 2 4 8
    do
        for((j=0;j<$num_run;j++))
        do
            if [ $test = 'cholesky' ]
            then
              echo  $1 $2 $3 $nthreads " ">>$logfile
              echo -n $1 $2 $3 $nthreads " " >>$outfile
			  if [ $1 = 'upch' ]
              then
                upcrun -n 1 ./${test}_$1 $2 $3 $nthreads >tmp
              else
                ./${test}_$1 $2 $3 $nthreads >tmp
              fi

            else
              echo $1 $niter $2 $3  $nthreads " ">>$logfile
              echo -n $1 $niter $2 $3  $nthreads " ">>$outfile
              if [ $1 = 'upch' ]
 			  then
                upcrun -n 1 ./${test}_$1 $niter $2 $3  $nthreads >tmp
             else
                ./${test}_$1 $niter $2 $3  $nthreads >tmp
			  fi
            fi
            grep "exec time : " tmp | awk '{print $4}' >> $outfile
            cat tmp >>$logfile
        done
        echo -n $1 $niter $2 $3  $nthreads average" ">>${test}_ave.log
        tail -n 5 $outfile >> tmp.log
        if [ $test = 'cholesky' ]
        then
          grep "$1" tmp.log  | awk 'BEGIN{sum=0;num=0}{sum+=$5;num+=1}END{print sum/num}' >> ${test}_ave.log
        else
          grep "$1" tmp.log  | awk 'BEGIN{sum=0;num=0}{sum+=$6;num+=1}END{print sum/num}' >> ${test}_ave.log
        fi
        rm tmp.log
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
    make clean
    rm tmp
}


testcase=( '2d5p' '3d7p' 'adifull' 'fdtd' 'cholesky')
flagname=("")
num_run=5
lenth=${#testcase[@]}

find  . -name *.log |xargs rm
find  . -name *.txt |xargs rm
rm -rf  exec_file.tar time
mkdir time
#echo "input value of MIC('TRUE' or empty):"
#read MIC
for((i=0;i<${lenth};i++))
do
  echo ${testcase[$i]}
  curdate=$(date +%Y_%m_%d_%H_%M_%S)
  outfile=${testcase[$i]}_$curdate.txt
  logfile=${testcase[$i]}_$curdate.log
  niter=40
  test=${testcase[$i]}
  cd ${TEST_HOME}/examples/${testcase[$i]}
  if [ "$MIC"x == "TRUE"x ]
  then 
    make MIC=TRUE 
    cp ${test}_omp  ${test}_dag  ${TEST_HOME}/examples/ 
  else
    case $test in
    '2d5p' )
      do_one_test omp 128 128
      do_one_test dag 4 4096
      do_one_test upch 16 4096
    ;;
    'fdtd' )
      do_one_test omp 256 256
      do_one_test dag 4 4096
      do_one_test upch 16 4096
    ;;
    '3d7p' )
      do_one_test omp 8 16
      do_one_test dag 16 16
      do_one_test upch 16 16
    ;;
    'adifull' )
      do_one_test omp 128 128 
      do_one_test dag 128 128
      do_one_test upch 128 128
    ;;
    'cholesky' )
      do_one_test omp 70 70
      do_one_test dag 70 70
    ;;
    esac
     cp  ${TEST_HOME}/examples/${testcase[$i]}/${test}_ave.log   ${TEST_HOME}/examples/time/
  fi
done
if [ "$MIC"x == "TRUE"x ]
then
cd  ${TEST_HOME}/examples/
tar czvf exec_file.tar *_omp *_dag
rm *_omp *_dag
scp exec_file.tar mic_test.sh  mic0:
fi

