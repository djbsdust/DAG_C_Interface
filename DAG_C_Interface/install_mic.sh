mkdir ~/dag_task_scheduler > /dev/null 2>&1
mkdir ~/dag_task_scheduler/include > /dev/null 2>&1
mkdir ~/dag_task_scheduler/lib > /dev/null 2>&1
mkdir ~/dag_task_scheduler/lib/mic > /dev/null 2>&1

cd /home/houxionghui/AceMeshRuntime/
make clean 
make DagTaskScheduler_static

cp /home/houxionghui/AceMeshRuntime/aceMesh_runtime.h /home/houxionghui/dag_task_scheduler/include/
cp /home/houxionghui/AceMeshRuntime/libdag_task_scheduler.a /home/houxionghui/dag_task_scheduler/lib/mic

