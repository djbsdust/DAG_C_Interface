#include <stdio.h>
#include <math.h>
#include <time.h>
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

//double t_start,t_end;


void init_array()
{
   int i, j;
   
   for(i = 0; i < nx+1; i++)
     for(j = 0; j < ny; j++)
        ey[i][j] = (double)((i+1)/(j+1));

   for(i = 0; i < nx; i++)
     for(j = 0; j < ny+1; j++)
        ex[i][j] = (double)(1/(i+j+1));


   for(i = 0; i < nx; i++)
     for(j = 0; j < ny; j++)
        hz[i][j] = (double)(1/(i+1)/(j+1));;
}

int main(int argc,char **argv)
{
   int t, i, j, k, m, n,jj;
  struct timeval start;
  struct timeval end;
  double totaltime;
   //int tz = 4;
//  int tz=4;  //default tile size
 // num_threads=4;  //default thread num
  //if(argc >1)
//	tz=atoi(argv[1]);
  gettimeofday(&start,NULL);
 
   init_array();

   for(t = 0; t < tmax; t++){

     for(j = 0; j < ny; j++)
        ey[0][j] = t;

    /*for (i=1; i<nx; i++)
        for (jj=0; jj<ny; jj=jj+tz)
		  for(j=jj; j<min(ny,jj+tz); j++)*/
   for (i=1; i<nx; i++)
     for(j = 0; j < ny; j++)
            ey[i][j] = ey[i][j] - 0.5*(hz[i][j]-hz[i-1][j]);
 
    /*for(i = 0; i < nx; i++)
       for(jj = 1; jj < ny; jj=jj+tz)
	  for(j=jj; j<min(ny,jj+tz); j++)*/
    for(i = 0; i < nx; i++)
      for(j = 1; j < ny; j++)
		 ex[i][j] = ex[i][j] - 0.5*(hz[i][j] - hz[i][j-1]);

    /*for(i = 0; i < nx; i++)
    
      // for(jj = 0; jj < ny; jj=jj+tz)
	//	  for(j=jj; j<min(ny,jj+tz); j++)*/
    for(i = 0; i < nx; i++)
     for(j = 0; j < ny; j++)
          hz[i][j] = hz[i][j] - 0.7*(ex[i][j+1] - ex[i][j] + ey[i+1][j] - ey[i][j]);
    }
 
   /* cout<<"this is matrix hz"<<endl;
   for(i=0;i<nx;i++)
     for(j=0;j<ny;j++){
       cout<<hz[i][j]<<" ";
       if (j%80 == 20) cout<<endl;
	   }
	   cout<<endl;
*/

    gettimeofday(&end,NULL);
	totaltime=(end.tv_sec-start.tv_sec)+ (end.tv_usec-start.tv_usec)*1.0e-6;
    //cout<<"fdtd-2d execute time ="<< totaltime<<endl;
   printf("fdtd-2d execute time = %f\n",totaltime);

   return 0;
}
