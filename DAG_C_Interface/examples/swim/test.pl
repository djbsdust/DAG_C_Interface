#!/usr/bin/perl -w

use strict;

#my $name1=$ARGV[0];
#my $name=$ARGV[1];
#if($name1 eq "dag" ){

my @threads=(1,2,4,8);
my $out_file1="dag-test.log";
my $input="input.3801";
if(-e $out_file1 && -e "time-dag.log"){
  print("rm  $out_file1 \n");
  system("rm $out_file1");
  print("rm time-dag.log\n");
  system("rm time-dag.log");
}



print("ifort -O -o dag-swim dag-swim.f -cpp -free -lstdc++  -L$ENV{DAG_HOME}/lib  -I$ENV{DAG_HOME}/include   -ldag_task_scheduler -ltbb \n");
system("ifort -O -o dag-swim dag-swim.f -cpp -free -lstdc++  -L$ENV{DAG_HOME}/lib  -I$ENV{DAG_HOME}/include   -ldag_task_scheduler -ltbb");
foreach my $j (0 .. $#threads){
   foreach my $i (0 .. 4){
     $ENV{"threads"}=$threads[$j];	
     open(LOG,">>$out_file1")||die("cant not open file:$!");
     print LOG "threads=$threads[$j],num=$i \n";
     close LOG;	 
     print("-----threads=$threads[$j],num=$i------\n");
     print("./dag-swim <$input >>$out_file1 \n");
     system("./dag-swim <$input >>$out_file1 ");
     }
}
    print("grep  -E \"threads|exec time :\" $out_file1 >time-dag.log\n");
    system("grep -E \"threads|exec time :\" $out_file1 >time-dag.log");
#}
#if($name eq "omp") {
############################omp test perl##################################################
my @OMP_NUM_THREADS=(1,2,4,8);
if(-e "omp-test.log" && -e "time-omp.log"){
  print("rm omp-test.log \n");
  system("rm omp-test.log");
  print("rm time-omp.log\n");
  system("rm time-omp.log");
}
print("ifort -O -o omp-swim omp-swim.f -cpp -free -openmp \n");
system("ifort -O -o omp-swim omp-swim.f -cpp -free -openmp");

foreach my $i (0 .. $#OMP_NUM_THREADS){
  foreach my $j (0 .. 4){
 #   export OMP_NUM_THREADS=$OMP_NUM_THREADS[$i];
    $ENV{"OMP_NUM_THREADS"}=$OMP_NUM_THREADS[$i];
    print("-----OMP_NUM_THREADS=$OMP_NUM_THREADS[$i],num=$j-----\n");
    print("./omp-swim <$input >>omp-test.log \n");
    system("./omp-swim <$input  >>omp-test.log");
  }
}
print("grep \"total time\" omp-test.log > time-omp.log \n");
system("grep \"total time\" omp-test.log >time-omp.log");
#}
