#!/bin/bash

SRC=./
DIR=~/
INSTALL_PATH=${DAG_HOME}
if [ ! -n "$INSTALL_PATH" ]
then
   echo "please set DAG_HOME"
   exit 1
fi
#/dag_task_scheduler
#_1level

if [ ! -d  ${INSTALL_PATH} ]  
then
   mkdir -p ${INSTALL_PATH}
fi

if [ ! -d  ${INSTALL_PATH}/include ]  
then
   mkdir ${INSTALL_PATH}/include
fi

if [ ! -d  ${INSTALL_PATH}/lib ]  
then
  mkdir ${INSTALL_PATH}/lib
fi

if [ ! -d  ${INSTALL_PATH}/bin ]  
then
   mkdir ${INSTALL_PATH}/bin
fi

pushd  ${INSTALL_PATH}/include
rm -f *
popd
pushd ${INSTALL_PATH}/lib
rm -f *
popd
pushd ${INSTALL_PATH}/bin
rm -f *
popd

cd ${SRC}

#make clean

#printing version info into aceMesh_runtime.cpp

#echo -n "string versionString1(\"" >>versionInfo
#svn log |sed -n '2p'|awk '{print $1,"\");"}'>>versionInfo
#echo -n "string versionString2(\"svn date: ">>versionInfo
#svn log |sed -n '2p'|awk '{print $5,$6"\");"}'>>versionInfo
sed -i '/string CompileTime/d'  versionInfo
echo -n "string CompileTime(\"" >>versionInfo
date "+%G-%m-%d %H:%M:%S\");">>versionInfo

sed '/string versionString/d; /string CompileTime/d'  aceMesh_runtime.cpp >foo
sed '/using std::string/r versionInfo' foo>aceMesh_runtime.cpp
rm foo

##finished modification to aceMesh_runtime.cpp

make -f makefile DagTaskScheduler_static
#make DagTaskScheduler_dynamic


cp ${SRC}/aceMesh_runtime_install.h ${INSTALL_PATH}/include/aceMesh_runtime.h
cp ${SRC}/aceMesh_runtime_c.h    ${INSTALL_PATH}/include/aceMesh_runtime_c.h
cp ${SRC}/aceMesh_runtime_f.h    ${INSTALL_PATH}/include/aceMesh_runtime_f.h
cp ${SRC}/aceMesh_task_install.h    ${INSTALL_PATH}/include/aceMesh_task.h
cp ${SRC}/aceMesh_concurrent_task_install.h    ${INSTALL_PATH}/include/aceMesh_concurrent_task.h
cp ${SRC}/concurrent_aceMesh_task.h    ${INSTALL_PATH}/include/
cp ${SRC}/task.h    ${INSTALL_PATH}/include/
cp ${SRC}/MANUAL.txt    ${INSTALL_PATH}/include/

#cp tools to bin
cp ${SRC}/tools/reuse_distance_merge.py   ${INSTALL_PATH}/bin/ 
cp ${SRC}/tools/reuse_distance.py   ${INSTALL_PATH}/bin/ 
cp ${SRC}/tools/merge_trace.py   ${INSTALL_PATH}/bin/ 
cp ${SRC}/tools/get_cpu_topo.py   ${INSTALL_PATH}/bin/ 
cp ${SRC}/tools/get_loop_info.py   ${INSTALL_PATH}/bin/ 
cp ${SRC}/tools/draw_dag_graph_with_pygraphviz.py   ${INSTALL_PATH}/bin/ 

chmod u+x  ${INSTALL_PATH}/bin/*

cp ${SRC}/DataRace.h   ${INSTALL_PATH}/include/ 
cp ${SRC}/WrapRep.h         ${INSTALL_PATH}/include/ 
cp ${SRC}/DataRaceTrace.h   ${INSTALL_PATH}/include/ 

#added by zhanghui
cp ${SRC}/aceMesh_hierarchy_task_install.h ${INSTALL_PATH}/include/aceMesh_hierarchy_task.h
cp ${SRC}/splitter.h ${INSTALL_PATH}/include/
cp ${SRC}/thread_group.h ${INSTALL_PATH}/include/
#cp ${SRC}/task_dag_graph.h ${INSTALL_PATH}/include/
#cp ${SRC}/trace_out.h ${INSTALL_PATH}/include/
#cp ${SRC}/dag_graph_check.h ${INSTALL_PATH}/include/
#cp ${SRC}/chombo_interface.h ${INSTALL_PATH}/include/
cp ${SRC}/range.h ${INSTALL_PATH}/include
#cp ${SRC}/tbb_thread_local.h ${INSTALL_PATH}/include

#cp ${SRC}/libdag_task_scheduler.so ${INSTALL_PATH}/lib/
cp ${SRC}/libdag_task_scheduler.a ${INSTALL_PATH}/lib/

cp ${SRC}/dag_struct.mod ${INSTALL_PATH}/include/
make clean
