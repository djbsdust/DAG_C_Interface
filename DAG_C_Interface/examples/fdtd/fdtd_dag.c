#include <stdio.h>
#include <math.h>
#include<omp.h> 
#include <stdlib.h>
#include<sys/time.h>

#include "aceMesh_runtime_c.h"
/*
#define tmax 400
#define nx 2048
#define ny 2048
*/

#define min(a,b) ((a>b)?(b):(a))
#define max(a,b) ((a>b)?(a):(b))

double ex[nx][ny+1];
double ey[nx+1][ny];
double hz[nx][ny];
double ex1[nx][ny+1]; 
double ey1[nx+1][ny]; 
double hz1[nx][ny];
int tz;

typedef struct {
    int t;
    void *addrs;
}targs;
typedef struct{
    int ii;
    void *addrs;
}TARGS_T2;
void stencil1(targs *each_y)
{
  int t_y=each_y->t;
  int j;
  for(j=0;j<ny;j++)
	ey[0][j]=t_y;
}
void stencil2(TARGS_T2 *args2)
{
  int ii=args2->ii;
  int i,j;
  for(i=max(ii,1);i<min(nx,ii+tz);i++)
   for (j=0; j<ny; j++)
	ey[i][j] = ey[i][j] - 0.5*(hz[i][j] - hz[i-1][j]);
}
void stencil3(TARGS_T2 *args3)
{
  int ii=args3->ii;
  int i,j;
   for (i=ii; i<min(nx,ii+tz); i++)
    for(j = 1; j < ny; j++)
     ex[i][j] = ex[i][j] - 0.5*(hz[i][j] - hz[i][j-1]);

}
void stencil4(TARGS_T2 *args4)
{
  int i,j;
  int ii=args4->ii;
  for(i=ii; i<min(nx,ii+tz); i++)
   for(j=0;j<ny;j++)
      hz[i][j] = hz[i][j] - 0.7*(ex[i][j+1] - ex[i][j] + ey[i+1][j] - ey[i][j]);
}
void neighbor_loop2(struct ptr_array *neighbor_addrs,TARGS_T2 *args2)
{
 int ii=args2->ii;
 void *addr=args2->addrs;
 neighbor_addrs->len=0;
 if(ii>=tz)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(addr-tz*ny*8);
 }

void neighbor_loop4(struct ptr_array *neighbor_addrs,TARGS_T2 *args4)
{
  int ii=args4->ii;
  void *addr1=args4->addrs;
   neighbor_addrs->len=0;
   if(ii<nx-tz)
       neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(addr1+tz*ny*8);
   
}

void init_array(double ex[][ny+1],double ey[][ny], double hz[][ny])
{
   int i, j;
   for(i = 0; i < nx+1; i++)
     for(j = 0; j < ny; j++)
        ey[i][j] = (double)((i+1)/(j+1));

   for(i = 0; i < nx; i++)
     for(j = 0; j < ny+1; j++)
        ex[i][j] =(double)(1/(i+j+1)); 


   for(i = 0; i < nx; i++)
     for(j = 0; j < ny; j++)
        hz[i][j] = (double)(1/(i+1)/(j+1));
   
}

int check()
{
   int i1,j1,t1;
	//YOU NEED INITIALIZATION HERE
	init_array(ex1,ey1,hz1);  //by lchen

  //........not parallel region and add hz1 for check.............	
  for(t1=0; t1<tmax; t1++)  {
        for (j1=0; j1<ny; j1++)
            ey1[0][j1] = t1;
        for (i1=1; i1<nx; i1++)
           for (j1=0; j1<ny; j1++)
                ey1[i1][j1] = ey1[i1][j1] - 0.5*(hz1[i1][j1]-hz1[i1-1][j1]);
        for (i1=0; i1<nx; i1++)
            for (j1=1; j1<ny; j1++)
                ex1[i1][j1] = ex1[i1][j1] - 0.5*(hz1[i1][j1]-hz1[i1][j1-1]);
        for (i1=0; i1<nx; i1++)
           for (j1=0; j1<ny; j1++)
                hz1[i1][j1]=hz1[i1][j1]-0.7*(ex1[i1][j1+1]-ex1[i1][j1]+ey1[i1+1][j1]-ey1[i1][j1]);
                
    }
//.............add it for check............

for(i1=0;i1<nx;i1++)
   for(j1=0;j1<ny;j1++)
   {
    if(fabs(hz1[i1][j1]-hz[i1][j1])>1e-10)
	{
	printf("< %d,%d >: ",i1,j1);
	printf("%f is incoistent with ",hz1[i1][j1]);
	printf("%f\n",hz[i1][j1]);
	 return 1;
	}
	
   }
	printf("......check is right !......\n");
	return 0;
}
int main(int argc, char** argv)
{
   int t,s, i, j,ii,jj, num_threads;
  targs each_y;
  struct timeval start;
  struct timeval end;
  double totaltime;
  tz=4;  //default tile size
  num_threads=4;  //default thread num
  TARGS_T2 args2, args3;
  TARGS_T2 args4;
  if(argc >1)
	tz=atoi(argv[1]);
  if(argc>2)
	num_threads =atoi(argv[2]);
  init_array(ex,ey,hz);
  
  acemesh_runtime_init(num_threads);
  gettimeofday(&start,NULL);
  
  /*..........parallel region............*/
  for(t = 0; t < tmax; t++){
    acemesh_begin_split_task("loop1");
	each_y.t=t;
	each_y.addrs=&ey[0][0];
	acemesh_push_wlist(1, &ey[0][0], NORMAL);
	acemesh_task_generator((TASK_FUNCPTR)stencil1,(void*)(&each_y),sizeof(targs));
	acemesh_task_set_type(STENCIL_TASK);
    acemesh_end_split_task();
    
	acemesh_begin_split_task("loop2"); 
   for (ii=0; ii<nx; ii+=tz){
     args2.ii=ii;
	 args2.addrs=&hz[ii][0];
	 acemesh_push_wrlist(1,&ey[ii][0],NORMAL);
	 acemesh_push_rlist(1, &hz[ii][0], NORMAL);
	 acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil2,
                      (void*)(&args2), sizeof(TARGS_T2), 
                      NULL, (NEIGHBOR_FUNCPTR)neighbor_loop2, &args2);
	 acemesh_task_set_type(STENCIL_TASK);
     }
    acemesh_end_split_task();
    acemesh_begin_split_task("loop3");
    for(ii = 0; ii < nx; ii+=tz){
       args3.ii=ii;
       args3.addrs=&hz[ii][0];
       acemesh_push_wrlist(1,&ex[ii][0],NORMAL);
       acemesh_push_rlist(1, &hz[ii][0], NORMAL);
       acemesh_task_generator((TASK_FUNCPTR)stencil3,(void*)(&args3), sizeof(TARGS_T2));
       acemesh_task_set_type(STENCIL_TASK);
     }
    acemesh_end_split_task();
    acemesh_begin_split_task("loop4");
    for(ii = 0; ii < nx; ii+=tz){
       args4.ii=ii;
       args4.addrs=&ey[ii][0];
       acemesh_push_wrlist(1,&hz[ii][0],NORMAL);
       acemesh_push_rlist(2, &ex[ii][0],NORMAL,&ey[ii][0],NORMAL);
       acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil4,
                      (void*)(&args4), sizeof(TARGS_T2), 
                      NULL, (NEIGHBOR_FUNCPTR)neighbor_loop4, &args4);
       acemesh_task_set_type(STENCIL_TASK);
    }
    acemesh_end_split_task();
   }
    acemesh_spawn_and_wait(1);
	gettimeofday(&end,NULL);
	totaltime=(end.tv_sec-start.tv_sec)+ (end.tv_usec-start.tv_usec)*1.0e-6;
   acemesh_runtime_shutdown(); 
#ifdef CHECK
   if(check()) 
   {
	printf("error!\n");
	exit(1);
   }
#endif
	printf("fdtd-2d,using %d threads\n,total_time : %f seconds\n",num_threads,totaltime);

   return 0;
}                       
