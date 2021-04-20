#include <stdio.h>
#include <math.h>
#include<omp.h> 
#include <stdlib.h>
#include<sys/time.h>

#include "aceMesh_runtime_c.h"


#define min(a,b) ((a>b)?(b):(a))
#define max(a,b) ((a>b)?(a):(b))
double ex[nx][ny+1];
double ey[nx+1][ny];
double hz[nx][ny];
double ex1[nx][ny+1]; 
double ey1[nx+1][ny]; 
double hz1[nx][ny];
int tz1,tz2,tmax;

typedef struct {
    int jj;
    int t;
    void *addrs;
}targs1;
typedef struct{
    int ii;
    int jj;
    void *addrs;
}targs2;
typedef struct{
    int ii;
    int jj;
    void *addr_ey;
    void *addr_ex;
}targs4;
void stencil_loop1(targs1 *args1)
{
  int j;
  
  int t_y=args1->t;
  int jj=args1->jj;
  for(j=jj;j<min(ny,jj+tz2);j++)
	ey[0][j]=t_y;
}
void stencil_loop2(targs2 *args2)
{
  int ii=args2->ii;
  int jj=args2->jj;
  int i,j;
  for(i=max(ii,1);i<min(nx,ii+tz1);i++)
   for(j=jj;j<min(ny,jj+tz2);j++)
	ey[i][j] = ey[i][j] - 0.5*(hz[i][j] - hz[i-1][j]);
}
void stencil_loop3(targs2 *args3)
{
  int ii=args3->ii;
  int jj=args3->jj;
  int i,j;
   for (i=ii; i<min(nx,ii+tz1); i++)
    for(j = max(jj,1); j < min(ny,jj+tz2); j++)
     ex[i][j] = ex[i][j] - 0.5*(hz[i][j] - hz[i][j-1]);

}
void stencil_loop4(targs2 *args4)
{
  int i,j;
  int ii=args4->ii;
  int jj=args4->jj;
  for(i=ii; i<min(nx,ii+tz1); i++)
    for(j = jj; j < min(ny,jj+tz2); j++)
      hz[i][j] = hz[i][j] - 0.7*(ex[i][j+1] - ex[i][j] + ey[i+1][j] - ey[i][j]);
}
void neighbor_loop2(struct ptr_array *neighbor_addrs,targs2 *args2)
{
 int ii=args2->ii;
 void *addr=args2->addrs;
 neighbor_addrs->len=0;
 if(ii>=tz1)
        neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(addr-tz1*ny*8);
 }

void neighbor_loop3(struct ptr_array *neighbor_addrs,targs2 *args3)
{
 int ii=args3->ii;
 int jj=args3->jj;
 void *addr=args3->addrs;
 neighbor_addrs->len=0;
  
  if(jj>=tz2)
     neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(addr-tz2*8);
}
void neighbor_loop4(struct ptr_array *neighbor_addrs,targs4 *args4)
{
  int ii=args4->ii;
  int jj=args4->jj;
  void *addr_ex=args4->addr_ex;//previous addr1
  void *addr_ey=args4->addr_ey;//previous addr2
   neighbor_addrs->len=0;
   if(ii<nx-tz1)
       neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(addr_ey+tz1*ny*8);
   if(ii>=tz2)
       neighbor_addrs->arr[(neighbor_addrs->len)++]=(void*)(addr_ex-tz2*8);
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
  targs1 args1;
  struct timeval start;
  struct timeval end;
  double totaltime;
  tz1=4;  //default tile size
  tz2=128;
  num_threads=4;  //default thread num
  targs2 args2;
  targs2 args3;
  targs4 args4;
  printf("USAGE: tmax tile_size1 tile_size2 num_threads\n");
  if(argc >1)
        tmax=atoi(argv[1]);
  if(argc >2)
	tz1=atoi(argv[2]);
  if(argc >3)
	tz2=atoi(argv[3]);
  if(argc>4)
	num_threads =atoi(argv[4]);
  init_array(ex,ey,hz);
  
    /*..........parallel region............*/
  acemesh_runtime_init(num_threads);
  gettimeofday(&start,NULL);
  for(t = 0; t < tmax; t++){
    acemesh_begin_split_task("loop1");
    for(jj = 0; jj < ny; jj+=tz2)
     {	
		 args1.t=t;
         args1.jj=jj;
		 args1.addrs=&ey[0][jj];
		 acemesh_push_wlist(1, &ey[0][jj], NORMAL);
		 acemesh_task_generator((TASK_FUNCPTR)stencil_loop1,(void*)(&args1),sizeof(targs1));
		 acemesh_task_set_type(STENCIL_TASK);
     }
    acemesh_end_split_task();
    acemesh_begin_split_task("loop2"); 
    for (ii=0; ii<nx; ii+=tz1)
    for (jj=0; jj<ny; jj+=tz2){
	    args2.ii=ii;
        args2.jj=jj;
	    args2.addrs=&hz[ii][jj];
	    acemesh_push_wrlist(1,&ey[ii][jj],NORMAL);
	    acemesh_push_rlist(1, &hz[ii][jj], NORMAL);
	    acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_loop2,
                      (void*)(&args2), sizeof(targs2), 
                      NULL, (NEIGHBOR_FUNCPTR)neighbor_loop2, &args2);
	    acemesh_task_set_type(STENCIL_TASK);
    }
    acemesh_end_split_task();
    acemesh_begin_split_task("loop3");
    for(ii = 0; ii < nx; ii+=tz1)
    for(jj=0;jj<ny;jj+=tz2){
        args3.ii=ii;
        args3.jj=jj;
        args3.addrs=&hz[ii][jj];
        acemesh_push_wrlist(1,&ex[ii][jj],NORMAL); 
        acemesh_push_rlist(1, &hz[ii][jj], NORMAL);
        acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_loop3,
                   (void*)(&args3), sizeof(targs2),
                   NULL, (NEIGHBOR_FUNCPTR)neighbor_loop3, &args3);
        acemesh_task_set_type(STENCIL_TASK);
    }
    acemesh_end_split_task();
    acemesh_begin_split_task("loop4");
    for(ii = 0; ii < nx; ii+=tz1)
    for(jj=0;jj<ny;jj+=tz2){
        args4.ii=ii;
        args4.jj=jj;
        args4.addr_ey=&ey[ii][jj];
        args4.addr_ex=&ex[ii][jj];
        acemesh_push_wrlist(1,&hz[ii][jj],NORMAL);
        acemesh_push_rlist(2, &ex[ii][jj],NORMAL,&ey[ii][jj],NORMAL);
        acemesh_task_generator_with_neighbors((TASK_FUNCPTR)stencil_loop4,
                      (void*)(&args4), sizeof(targs4), 
                      NULL, (NEIGHBOR_FUNCPTR)neighbor_loop4, &args4);
        acemesh_task_set_type(STENCIL_TASK);
    }
    acemesh_end_split_task();
  } //time iteration
  acemesh_spawn_and_wait(1);
  gettimeofday(&end,NULL);
  totaltime=(end.tv_sec-start.tv_sec)+ (end.tv_usec-start.tv_usec)*1.0e-6;
  acemesh_runtime_shutdown(); 
#ifdef CHECK
   if(check()) 
   {
	printf("exec time = check_error!\n");
	exit(1);
   }
#endif
	printf("fdtd-2d,using %d threads,total time : %f seconds\n",num_threads,totaltime);

   return 0;
}                       
