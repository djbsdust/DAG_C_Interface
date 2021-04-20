#!bin/sh

MIC=TRUE

#$1, cdag/omp/fdag
#$2, tile_sz1
#$3, tile_sz2
function test_one_parameter()
{
    for nthreads in 16 32 57 114 171
    do
        for((j=0;j<$num_run;j++))
        do
            if [ $test = 'cholesky' ]
            then
              echo  $1 $2 $3 $nthreads " ">>$logfile
              echo -n $1 $2 $3 $nthreads " ">>$outfile
              ./${test}_$1 $2 $3 $nthreads >tmp
            else
              echo $1 $niter $2 $3  $nthreads " ">>$logfile
              echo -n $1 $niter $2 $3  $nthreads " ">>$outfile
              ./${test}_$1 $niter $2 $3  $nthreads >tmp
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
#$2, tile_sz1                                                                                               
#$3, tile_sz2                                                                                               
function do_one_test()                                                                                      
{                                                                                                           
    test_one_parameter $1 $2 $3                                                                             
}                                                                                                           
                                                                                                            
                                                                                                            
testcase=('2d5p' '3d7p' 'adifull' 'fdtd' 'cholesky')                                                        
flagname=("")                                                                                               
num_run=5                                                                    
lenth=${#testcase[@]}


rm *.txt *.log                                                                                              
tar zxvf exec_file.tar                                                                                      
for((i=0;i<${lenth};i++))                                                                                   
do                                                                                                          
  echo ${testcase[$i]}                                                                                      
  curdate=$(date +%Y_%m_%d_%H_%M_%S)                                                                        
  outfile=${testcase[$i]}_$curdate.txt                                                                      
  logfile=${testcase[$i]}_$curdate.log                                                                      
  niter=40                                                                                                  
  test=${testcase[$i]}                                                                                      
  case $test in                                                                                             
    '2d5p' | 'fdtd' )                                                                                       
      do_one_test omp 128 256                                                                               
      do_one_test dag 16 4096                                                                              
    ;;                                                                                                      
    '3d7p' )                                                                                                
      do_one_test omp 16 16                                                                                  
      do_one_test dag 16 16                                                                                
    ;;                                                                                                      
    'adifull' )                                                                                             
      do_one_test omp 128 128                                                                               
      do_one_test dag 128 128                                                                              
    ;;                                                                                                      
    'cholesky' )                                    
      do_one_test omp 70 70                         
      do_one_test dag 70 70                        
    ;;                                              
   esac                                             
done
