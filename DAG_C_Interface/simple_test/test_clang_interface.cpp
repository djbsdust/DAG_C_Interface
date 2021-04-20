#include "aceMesh_runtime.h"

#define M 1024
#define BLOCK 16

#define for_block(s) for(int i = (0 + (s)); i < ((s) + BLOCK); i++)
#define shift(ii) ((ii)+BLOCK)%M

using namespace std;

typedef struct
{
	double *a;
	double *b;
	int j;
}targs; 

void for_job_1(void *a_args)
{
  targs *args = (targs *)a_args;
	
  for_block(0){
    (args->a)[i] = (args->a)[i] * (args->b)[i] * (args->b)[i] * (args->b)[i];
  }
  for(int i = 0; i < 10000; i++);
}

void for_job_2(void *a_args)
{
  targs *args = (targs *)a_args;
	
  for_block(0){
    (args->a)[i] = (args->b)[i] * (args->b)[i] * (args->b)[i];
  }
  for(int i = 0; i < 10000; i++);
}

// test_ci_task is to test intefaces:
// __
void test_ci_task(void)
{
  double A[M], AS[M], B[M], C[M], CS[M];

  for (int i = 0; i<M; i++)
  {
    A[i] = 0.1*i;
    AS[i] = 0.1*i;
    B[i] = 0.01*i;
  }
  
/*Serial code */
// #pragma acemesh for 
  for (int j = 0; j<M; j+=BLOCK)
  {
    double *a, *b;
    a = &AS[j];
    b = &B[j];
// #pragma acemesh data(a:RW,b:R)
    {
      for_block(0){
        a[i] = a[i]*b[i]*b[i]*b[i];
      }
    }
  }

// #pragma acemesh for 
  for (int k = 0; k<M; k+=BLOCK)
  {
    double *a, *b;
    int j = shift(k);
    a = &CS[k];
    b = &AS[j];
// #pragma acemesh data(a:RW,b:R)
    {
      for_block(0){
        a[i] = b[i]*b[i]*b[i];
      }
    }
  }

/*dag codes*/
  begin_split_task();
  for (int j = 0; j<M; j+=BLOCK)
  {
    targs args;
    args.a = &A[j];
    args.b = &B[j];

    __acemesh_push_wrlist(1, &A[j], UNSHADE);
    __acemesh_push_rlist(1, &B[j], UNSHADE);
    __acemesh_task_generator(for_job_1, &args, sizeof(args)); 
  }
  end_split_task();


  begin_split_task();
  for (int k = 0; k<M; k+=BLOCK)
  {
    int j = shift(k);
    targs args;
    args.a = &C[k];
    args.b = &A[j];

    __acemesh_push_wlist(1, &C[k], UNSHADE);
    __acemesh_push_rlist(1, &A[j], UNSHADE);
    __acemesh_task_generator(for_job_2, &args, sizeof(args));
  }
  end_split_task();

  spawn_and_wait(1);

/*Chech the results*/
  for(int i = 0; i < M; i++){
      if (CS[i] != C[i]){
        std::cout<<"FAILED"<<std::endl;
        return;
      }
  }
  
  std::cout<<"PASSED"<<std::endl;

}


/* This case is to test 
 * task_generator_with_neighbors
 * It is 1D3P stencil kernel
 */
 
 void for_job_3(void *a_args)
 {
	targs *_args = (targs *)a_args;
	int j = _args->j;
	double *lhs = _args->a;
	double *rhs = _args->b;
	
	for_block(j)
    {
		if (i>0 && i<M-1)
			lhs[i] = rhs[i] + 5.0*(rhs[i-1]+rhs[i+1]);
		if (i == 0)
			lhs[i] = rhs[i] + 5.0*(rhs[M-1]+rhs[i+1]);
		if (i == M-1)
			lhs[i] = rhs[i] + 5.0*(rhs[i-1]+rhs[0]);
		
		//cout<<lhs[i]<<"\t";
   }
   //cout<<endl;
   
}
 
 void get_sth(std::vector<void*>& neighbor_addrs, void* argv)
 {
	targs *args = (targs *)argv;
	//cout<<args->j<<" ";
	if (args->j > 0 && (args->j+BLOCK)< M)
	{
		//cout<<args->j-BLOCK<<","<<args->j+BLOCK;
		neighbor_addrs.push_back(&(args->b)[args->j-BLOCK]);
		neighbor_addrs.push_back(&(args->b)[args->j+BLOCK]);
	}
	if (args->j == 0)
	{
		//cout<<M-BLOCK<<","<<args->j+BLOCK;
		neighbor_addrs.push_back(&(args->b)[M-BLOCK]);
		neighbor_addrs.push_back(&(args->b)[args->j+BLOCK]);
	}
	if (args->j == M-BLOCK)
	{
		//cout<<args->j-BLOCK<<","<<0;
		neighbor_addrs.push_back(&(args->b)[args->j-BLOCK]);
		neighbor_addrs.push_back(&(args->b)[0]);
	}
	//cout<<endl;
 }
 
void test_ci_neighbors()
{
  double A[M], AS[M], B[M], BS[M];
  
  for (int i = 0; i<M; i++)
  {
    A[i] = 0.1*i;
    AS[i] = 0.1*i;
    B[i] = 0.01*i;
	BS[i] = 0.01*i;
  }

//Seriel code with pragmas
  int ts = 100;
  double *lhs, *rhs;
  lhs = AS;
  rhs = BS;

  while(ts-- > 0)
  {
//#pragma acemesh for  
    for(int j = 0; j < M ; j += BLOCK)
    {
	  double *lhs_b, *rhs_b;
      lhs_b = &lhs[j];
	  rhs_b = &rhs[j];
//#pragma acemesh data(lhs_b:W,rhs_b:R) commop(get_neighbors)
	{
	  for_block(j)
      {
	    //std::cout<<i+j<<std::endl;
		if (i>0 && i<M-1)
			lhs[i] = rhs[i] + 5.0*(rhs[i-1]+rhs[i+1]);
		if (i == 0)
			lhs[i] = rhs[i] + 5.0*(rhs[M-1]+rhs[i+1]);
		if (i == M-1)
			lhs[i] = rhs[i] + 5.0*(rhs[i-1]+rhs[0]);
		
		//cout<<lhs_b[i]<<"\t";
      }
	  //cout<<endl;
	 }
    }
	
	double *temp = lhs;
	lhs = rhs;
	rhs = temp;
  }
  
  lhs = A;
  rhs = B;
  ts = 100;
  while(ts-- > 0)
  {
    begin_split_task();
    for(int j = 0; j < M ; j+=BLOCK)
    {
	  targs a_args;
	  a_args.a = &lhs[0];
	  a_args.b = &rhs[0];
	  a_args.j = j;
	  
	  __acemesh_push_wlist(1, &lhs[j], UNSHADE);
      __acemesh_push_rlist(1, &rhs[j], UNSHADE);
	  __acemesh_task_generator_with_neighbors(for_job_3, &a_args, sizeof(a_args),
	                                          get_sth, (void *)(&a_args));
    }
	end_split_task();
	
	double *temp = lhs;
	lhs = rhs;
	rhs = temp;
  }
  spawn_and_wait(1);
  
  /*Chech the results*/
  for(int i = 0; i < M; i++){
      if (AS[i] != A[i]){
        std::cout<<"A["<<i<<"]="<<A[i];
		std::cout<<" AS["<<i<<"]="<<AS[i]<<" FAILED "<<std::endl;
        return;
      }
  }
  
  std::cout<<"PASSED"<<std::endl;

}
int main(void)
{
	aceMesh_runtime_init(4);
    test_ci_task();
	test_ci_neighbors();
}
