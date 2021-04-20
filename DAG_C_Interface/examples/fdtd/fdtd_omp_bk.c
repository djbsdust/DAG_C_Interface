#include <stdio.h>
#include <math.h>
#include<omp.h> 
#include <stdlib.h>
#include<sys/time.h>

/*#define tmax 400
#define nx 2048
#define ny 2048*/
#define min(a,b) ((a>b)?(b):(a))
int tmax;
double ex[nx][ny+1];
double ey[nx+1][ny];
double hz[nx][ny];
double ex1[nx][ny+1]; 
double ey1[nx+1][ny]; 
double hz1[nx][ny];

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
	printf("%f is inconsistent with ",hz1[i1][j1]);
	printf("%f\n",hz[i1][j1]);
	 return 1;
	}
	
   }
	printf("......check is right !......\n");
	return 0;
}
int main(int argc, char** argv)
{
   int t, i, j,ii,jj, num_threads;
  struct timeval start;
  struct timeval end;
  double totaltime;
  
  int tz1=128;  //default tile size
  int tz2=128;
  num_threads=4;  //default thread num
  printf("USAGE: tmax tile_size1 tile_size2 num_threads\n");
  if(argc >1)
        tmax=atoi(argv[1]);
  if(argc >2)
	tz1=atoi(argv[2]);
  if(argc >3)
	tz2=atoi(argv[3]);
  if(argc>4)
	num_threads =atoi(argv[4]);
  omp_set_num_threads(num_threads);

  init_array(ex,ey,hz);
  gettimeofday(&start,NULL);
    /*..........parallel region............*/
  for(t = 0; t < tmax; t++){
#pragma omp parallel shared(ex,ey,hz) private(i,j)
{ 
  #pragma omp for  
    for(jj = 0; jj < ny; jj+=tz2)
       for(j=jj;j<min(ny,jj+tz2);j++)
         ey[0][j] = t;
 
 
 #pragma omp for  
   for (ii=1; ii<nx; ii+=tz1)
    for(jj=0;jj<ny;jj+=tz2)
     for (i=ii; i<min(nx,ii+tz1); i++)
        for (j=jj; j<min(ny,jj+tz2); j++)
            ey[i][j] = ey[i][j] - 0.5*(hz[i][j]-hz[i-1][j]);
	
 #pragma omp for	
	for(ii = 0; ii < nx; ii+=tz1)
	for(jj=1;jj<ny;jj+=tz2)
         for (i=ii; i<min(nx,ii+tz1); i++)
          for(j = jj; j <min(ny,jj+tz2); j++)
           ex[i][j] = ex[i][j] - 0.5*(hz[i][j] - hz[i][j-1]);	
   
 #pragma omp for  
    for(ii = 0; ii < nx; ii+=tz1)
     for(jj=0;jj<ny;jj+=tz2)
      for (i=ii; i<min(nx,ii+tz1); i++)
       for(j = jj; j < min(ny,jj+tz2); j++)
          hz[i][j] = hz[i][j] - 0.7*(ex[i][j+1] - ex[i][j] + ey[i+1][j] - ey[i][j]);
   }
   }		
	gettimeofday(&end,NULL);
	totaltime=(end.tv_sec-start.tv_sec)+ (end.tv_usec-start.tv_usec)*1.0e-6;
#ifdef CHECK
   if(check()) 
   {
	printf("error!\n");
    //cout<<"error!"<<endl;
	exit(1);
   }
#endif
	printf("fdtd-2d,using %d threads\nexec time : %f seconds\n",num_threads,totaltime);
	//cout<<"fdtd-2d,using "<<num_threads<<"threads, execute time ="<< totaltime<<" seconds"<<endl;

   return 0;
}                       
