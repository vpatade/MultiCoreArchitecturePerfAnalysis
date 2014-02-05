/**
      Code : Matrix Multiplication Using Threads
      Name : Vallabh Patade
      ASU ID : 1206277484
      
*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

/* change d size as needed */
int dimension = 1024;
int NO_OF_RUNS = 2;
int NO_OF_THREADS;

//Arguments that we will pass to threads
typedef struct {
    double* A;
    double* B;
    double* C;
    int lower;
    int upper;
    int d;
    int t_id;
} my_args;

// Actual matrix multiplication for designated block of rows
void mult_matrix(double* A, double* B, double* C, int lower, int upper, int d) {
	int i,j,k;
	for(i = lower; i < upper; i++) {
		for(j = 0; j < d; j++) {
			for(k = 0; k < d; k++) {
					C[d*i+j] += A[d*i+k] * B[d*k+j];
			}
		}
	}
}

//Thread which will start seperate control of execution for matrix multiplication.
void* matrix_mult_thread(void *thread_arg) {
	my_args* arg = (my_args*)thread_arg;
	struct timeval begin, end;
	
	gettimeofday(&begin, NULL);								
	mult_matrix(arg->A,arg->B,arg->C,arg->lower,arg->upper,arg->d);
	gettimeofday(&end, NULL);
	printf("\n\t\t\t\tThread %d took %ldus", arg->t_id,((end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec)));
}

void multMatrixParallel(double* A, double* B, double* C, int d) {
	int i, lower, block_size;
	struct timeval begin, end;
	/*
	  Will create 1 less than NO_OF_THREADS bacause one thread will be master thread
	  So if total threads are n then (n-1) pthreads and 1 master thread
	*/
	
	//Arguments array to pass arguments to thread
	my_args* args = (my_args*)malloc( (NO_OF_THREADS-1)*sizeof(my_args));
	
	//Array of thread IDs
	pthread_t* t_ids = (my_args*)malloc( (NO_OF_THREADS-1)*sizeof(pthread_t));
	
	lower = 0;
	block_size = dimension/NO_OF_THREADS;
      
	//Bind the argument and pass it as a parameter for starting a thread
	for(i = 0; i < (NO_OF_THREADS-1); i++) {
	    args[i].A=A;
	    args[i].B=B;
	    args[i].C=C;
	    args[i].lower=lower;
	    args[i].upper = lower+block_size;
	    args[i].d = d;
	    args[i].t_id = i+1;
	    pthread_create(&t_ids[i],NULL,matrix_mult_thread,(void*)&args[i]);
	    lower += block_size;
	}
	
	//This call will be from Master thread.
	gettimeofday(&begin, NULL);
	mult_matrix(A,B,C,lower,d,d);
	gettimeofday(&end, NULL);
	printf("\n\t\t\t\tMaster Thread took %ldus",((end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec)));
	
	//Wait for all the thread to return.
	for(i = 0; i < (NO_OF_THREADS-1); i++) {
	    pthread_join(t_ids[i],NULL);
	}
	
	//Free the Dynamically Allocated Memory
	free(args);
	free(t_ids);
}


//utility function to set Dimensions and No fo runs variables.
void setSizeAndNofRuns(int input) {
      if(input > 8) {
	  dimension = input;
      } else if(input > 0){
	  NO_OF_RUNS = input;
      }
}

//Driver Function.
/*
   User can run the program by passing 0 or 1 or 2 arguments
   
   Two arguments are Dimension and No of Arguments. At a time you may enter only one argument or none.
   Dimension has to be more than 8 and no fo run has to be more than zero else default configuration will be picked up.
   
   Default Configuraiton :
                           No of Runs = 2
                           Dimension = 1024
*/
int main(int argc, char *argv[])
{
	int d, i, j, k, run, thread;
	double *A, *B, *C;
	long avg_time;
	struct timeval begin, end;

	for(i = 0; i < argc; i++) {
	    printf("\t%s",argv[i]);
	}
	
	//Check how many arguments and accordingly se the global variables
	if (argc == 2) {
		j = atoi(argv[1]);
		setSizeAndNofRuns(j);
		
	} else if(argc == 3) {
		j = atoi(argv[1]);
		k = atoi(argv[2]);
		
		setSizeAndNofRuns(j);
		setSizeAndNofRuns(k);
	}

	printf("\nDimension = %d\n", dimension);
	printf("No of Runs = %d\n", NO_OF_RUNS);
	
	srand(292); 
	long oneThreadTiming = 0;
	
	//Will run with  1,2,3......8 threads.
	for(thread=1;thread<=8;thread++) {
		NO_OF_THREADS = thread;
		printf("\n\tNumber of Threads = %d",NO_OF_THREADS);
		for (d = dimension ; d < dimension+1; d=d+128 ) {
		        //Vector to keep track of time required during each run. Used for calculating the average.
			long* time_record = (long*)malloc(NO_OF_RUNS*sizeof(long));
			
			for (run = 0 ; run < NO_OF_RUNS; run++) {
				printf("\n\t\tRun#%d",run+1);
				A = (double*)malloc(d*d*sizeof(double));
				B = (double*)malloc(d*d*sizeof(double));
				C = (double*)malloc(d*d*sizeof(double));
				
				//Go for initialize the Matrices.
				gettimeofday(&begin, NULL); /* returns the wall clock time */
				for(i = 0; i < d; i++) {
					for(j = 0; j < d; j++) {
						A[d*i+j] = (rand()/(RAND_MAX - 1.0));
						B[d*i+j] = (rand()/(RAND_MAX - 1.0));
						C[d*i+j] = 0.0;
					}
				}
				gettimeofday(&end, NULL);
				printf("\n\t\t\t[%d/%d] init loop took %ldus", d, run+1, ((end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec)));

				//Go for multiplication
				gettimeofday(&begin, NULL);
				multMatrixParallel(A,B,C,d);
				gettimeofday(&end, NULL);
				printf("\n\t\t\t[%d/%d] %ldus", d, run+1, ((end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec)));

				//Record the time required for this run
				time_record[run] = ((end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec));
				
				//free the memory chunks
				free(A);
				free(B);
				free(C);				
			}
			
			//Calculate and print the average time with these many threads
			long total=0;
			for(run=0;run<NO_OF_RUNS;run++) {
			    total+=time_record[run];
			}
			avg_time = (long)total/(long)NO_OF_RUNS;
			if(oneThreadTiming == 0) {
			  oneThreadTiming = avg_time;
			}
			printf("\n\t\t Average Time with %d Threads is %ldus\n",NO_OF_THREADS,avg_time);
			printf("\n\t\t***** Speed Up %.2fx",(double)oneThreadTiming/(double)avg_time);
			printf("\n---------------------------------------------------------------------------------------------------------------\n");
			
			//Free the memory allocated for time record vector
			free(time_record);
		}
	}

	return 0;
}
