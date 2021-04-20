#include <stdio.h>
#include <math.h>
#include<omp.h> 
#include <stdlib.h>
#include<sys/time.h>
//using namespace std;

/*#define tmax 400
#define nx 2048
#define ny 2048*/
#define min(a,b) ((a>b)?(b):(a))

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
        ey[i][j] = (double)(i+1)/(double)(j+1);//double((i+1)/(j+1));

   for(i = 0; i < nx; i++)
     for(j = 0; j < ny+1; j++)
        ex[i][j] =(double)1/(double)(i+j+1); //double(1/(i+j+1));


   for(i = 0; i < nx; i++)
     for(j = 0; j < ny; j++)
        hz[i][j] = (double)1/(double)(i+1)/(double)(j+1);
   
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
	// cout<<"<"<<i1<<","<<j1<<">:";
	 //cout<<hz1[i1][j1]<<" is incoistent with ";
	 //cout<<hz[i1][j1]<<endl;
	 return 1;
	}
	
   }
	printf("......check is right !......\n");
    //cout<<"......check is right !......"<<endl;
	return 0;
}
int main(int argc, char** argv)
{
   int t, i, j,ii, num_threads;
  struct timeval start;
  struct timeval end;
  double totaltime;
  
  int tz=4;  //default tile size
  num_threads=4;  //default thread num
  if(argc >1)
	tz=atoi(argv[1]);
  if(argc>2)
	num_threads =atoi(argv[2]);
  omp_set_num_threads(num_threads);

  init_array(ex,ey,hz);
  gettimeofday(&start,NULL);
    /*..........parallel region............*/
  for(t = 0; t < tmax; t++){
#pragma omp parallel shared(ex,ey,hz) private(i,j)
{ 
  #pragma omp for  
    for(j = 0; j < ny; j++)
        ey[0][j] = t;
 
 
 #pragma omp for  
   for (ii=1; ii<nx; ii+=tz)
     for (i=ii; i<min(nx,ii+tz); i++)
        for (j=0; j<ny; j++)
            ey[i][j] = ey[i][j] - 0.5*(hz[i][j]-hz[i-1][j]);
	
 #pragma omp for	
	for(ii = 0; ii < nx; ii+=tz)
      for (i=ii; i<min(nx,ii+tz); i++)
       for(j = 1; j < ny; j++)
          ex[i][j] = ex[i][j] - 0.5*(hz[i][j] - hz[i][j-1]);	
   
 #pragma omp for  
    for(ii = 0; ii < nx; ii+=tz)
      for (i=ii; i<min(nx,ii+tz); i++)
       for(j = 0; j < ny; j++)
          hz[i][j] = hz[i][j] - 0.7*(ex[i][j+1] - ex[i][j] + ey[i+1][j] - ey[i][j]);
   }
   }		
    /* cout<<"this is matrix hz"<<endl;
       #pragma omp parallel  for private(i,j) schedule(static)
           for(i=0;i<nx;i++)
                for(j=0;j<ny;j++){
                       cout<<hz[i][j]<<"  ";
                              if (j%80 == 20) cout<<endl;
                              	   }
                              	   	   cout<<endl; */
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
	printf("fdtd-2d,using %d threads\n exec time : %f seconds\n",num_threads,totaltime);
	//cout<<"fdtd-2d,using "<<num_threads<<"threads, execute time ="<< totaltime<<" seconds"<<endl;

   return 0;
}                       
