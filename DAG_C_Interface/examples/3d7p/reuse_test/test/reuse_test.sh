#!/bin/bash

THREAD=8
ITER=100
LOGDIR=./log

function once() {
    echo now starting ./$1 $2 $3 $4 $5 | tee -a $LOGDIR/${TIME}.$1.log
    ./$1 $2 $3 $4 $5 |& tee -a $LOGDIR/${TIME}.$1.log
}


function runtest() {
    touch $LOGDIR/${TIME}.$1.log
    #for thread in {1..3}
    #do
        for j in {2..5}
        do
            BLK=$((1<<$j))
            for repet in {1..5}
            do
                #for iter in {9..10}
                #do
                    once $1 $ITER $BLK $BLK $THREAD
                #done
            done
        done
    #done
}

function eachver() {
	for ver in "$@"
	do
		runtest $ver
	done
}

function compile() {
	for ver in "$@"
	do
		icpc -O3 -DSIZEX=512 -DSIZEY=512 -DSIZEZ=128 -Wall -I${DAG_HOME}/include -o $ver $ver.c -L${DAG_HOME}/lib -ldag_task_scheduler -ltbb -lrt
		#icpc -DCHECK -O3 -DSIZEX=512 -DSIZEY=512 -DSIZEZ=128 -Wall -I/home/export/online1/nsccwuxi_ict/chenl/lyt/dag_install_reuse/include -o $1 $1.c -L/home/export/online1/nsccwuxi_ict/chenl/lyt/dag_install_reuse/lib -ldag_task_scheduler -ltbb -lrt
	done
}


#编译
pushd ../
#compile 3d7p_dag{,_reuse,_multi,_once,_oncev1}
compile 3d7p_dag_multi_cmp
popd

#复制可执行文件到当前文件夹
#cp ../3d7p_dag{,_reuse,_multi,_once,_oncev1} ./
cp ../3d7p_dag_multi_cmp ./
#获取当前时间
TIME=$(date  +"%Y-%m-%d-%H-%M-%S")
mkdir -p $LOGDIR

#eachver 3d7p_dag{,_reuse,_multi,_once,_oncev1}
eachver 3d7p_dag_multi_cmp
