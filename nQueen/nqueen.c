/**
      Code : nQueen using parallel depth threads and worker threads
      Name : Vallabh Patade
      ASU ID : 1206277484

*/
#include <assert.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define N_THREADS 8
#define Q_CAPACITY 1000

int N_THREADS = 2;

/* Global static variable to keep track of how many threads are created and how many work is generated
and added to the queue for that perticular depth */
static int THREADS=0;
static int AMT_WORK=0;



/************ SERIAL APPROACH ************/
/**** column, left_diag and right_diag are bitmaps that indicate which positions are blocked ****/
int nq_serial(unsigned int col, unsigned int left, unsigned int right, int row, int n)
{
  if (row == n) {
    return 1;
  } else {
    int count = 0;
    unsigned int block = col | left | right;
    while (block + 1 < (1 << n)) {
      unsigned int open = (block + 1) & (~block); //Calculate possibility vector
      count += nq_serial(col | open,
	      ((left | open) << 1) & ((1 << n) - 1),  //Update left diagonal vector
	      (right | open) >> 1,  //Update right diagonal vector
	      row + 1,
	      n);
      block |= open; //Place the queen
    }
    return count;
  }
}


/*********** THREADS **************/

//Structure to pass arguments to the thread
typedef struct nqueen_arg {
    pthread_t t_id;
    unsigned int col;
    unsigned int left_dia;
    unsigned int right_dia;
    int row;
    int n;
    int parallel_depth;
    int count;
} nqueen_arg, * nqueen_arg_t;

void * nq_parallel_thread(void * arg);

//Thread which will work on it's assignmed work
void * nq_parallel_thread(void* arg) {
  nqueen_arg_t t_arg = (nqueen_arg_t)arg;
  t_arg->count = nq_parallel(t_arg->col, t_arg->left_dia, t_arg->right_dia, t_arg->row, t_arg->n, t_arg->parallel_depth);
  return 0;
}


/**** Main procedure for threads approach ****/
int nq_parallel(unsigned int col, unsigned int left, unsigned int right,
	int row, int n,
	int parallel_depth)
{
	if (row == n) {
	    return 1;
	} else {
		int n_threads = 0;
		nqueen_arg_t args = 0;
		unsigned int block = col | left | right;
		int count = 0;
		while (block + 1 < (1 << n)) {
			unsigned int open = (block + 1) & (~block); //Calculate the Possibility Vector
			if (row < parallel_depth) {
				/****** spawn thread up to parallel depth ******/
				assert(n_threads < n);
				if (args == 0) {
					args = (nqueen_arg_t)malloc(sizeof(nqueen_arg) * n);

				}

				//Every thread have a dedicated location for thier arguments in args array
				//Point to that location and update the arguments.
				args += n_threads;
				args->col = col | open; // Update the Column vector as per the new board state
				args->left_dia = ((left | open) << 1) & ((1 << n) - 1); //Update left diagonal vector as per the new board state
				args->right_dia = (right | open) >> 1;  //Update right diagonal diagonal vector as per the new board state
				args->row = row + 1; //Go on trying for next row
				args->n = n;
				args->parallel_depth = parallel_depth;
				pthread_create(&args->t_id, NULL, nq_parallel_thread, args);
				THREADS++;

				//get argument againt to the base address of the array
				args -= n_threads;

				n_threads++;
			}
			else {
				//Beyond the parallel depth use sequential approach
				count += nq_parallel(col | open,((left | open) << 1) & ((1 << n) - 1),(right | open) >> 1,row+1,n,parallel_depth);
			}
			block |= open; //Place the Queen
		}
		/****** wait for termination of spawned threads ******/
		int i;
		for (i = 0; i < n_threads; i++) {
			pthread_join(args[i].t_id,NULL);
			count += args[i].count;  //Sum up all the no of solutions that different threads have calculated
		}

		free(args); // Free the args array used for the threads at this perticular depth
    	return count;
	}
}

/****** END of THREADS ******/


/****** THREADS WITH WORK QUEUE ******/

/* data structure representing a queue of work to be done
   (a unit of work is represented by args to nqueen procedures) */
typedef struct work_queue {
  pthread_mutex_t mx;
  int head;
  int size;
  int capacity;
  nqueen_arg_t args;
} work_queue, *work_queue_t;


/* Function which will add one uni of work to the queue */
void work_queue_add(work_queue_t wq, unsigned int col, unsigned int left, unsigned int right, int row, int n, int parallel_depth) {
	int sz = wq->size;
	int capacity = wq->capacity;
	nqueen_arg_t args = wq->args;

    //If we are reaching the size of the queue, we will create another queu with double size.
	if (sz >= capacity) {
		int new_capacity = capacity * 2;
		if (new_capacity <= capacity) new_capacity = capacity + 1;
		nqueen_arg_t new_args = (nqueen_arg_t)malloc(sizeof(nqueen_arg) * new_capacity);
		// Copy all arguments of existing queue to the new queue with updated capacity
		bcopy(args, new_args, sizeof(nqueen_arg) * capacity);
		wq->capacity = capacity = new_capacity;
		wq->args = args = new_args;
	}
	assert(sz < capacity);
	nqueen_arg_t arg = args + sz; //Set the arguments of the thread at it's respective location in args array
	arg->col = col;
	arg->left_dia = left;
	arg->right_dia = right;
	arg->row = row;
	arg->n = n;
	arg->parallel_depth = parallel_depth;

	wq->size = sz + 1; //Increase the size of queus as new work is added
}

/* Function which makes a work queue of size CAPACITY*/
work_queue_t mk_work_queue() {
	int capacity = Q_CAPACITY;

	//Create the work queue
	work_queue_t wq = (work_queue_t)malloc(sizeof(work_queue));

	//create the args array which long enough.
	nqueen_arg_t args = (nqueen_arg_t)malloc(sizeof(nqueen_arg) * capacity);
	assert(wq);
	assert(args);
	// Initialize the mutex
	pthread_mutex_init(&wq->mx, 0);
	wq->head = 0; //Initial queue front and rear configuration
	wq->size = 0;
	wq->capacity = capacity;
	wq->args = args;

	return wq;
}

/* Function which add owrk to the queue upto defined depth and then calculate next lower levels serially.*/
int nq_work(work_queue_t wq, unsigned int col, unsigned int left, unsigned int right, int row, int n, int parallel_depth) {
	if (row == n) {
	      return 1;
	} else if (row == parallel_depth) {
	      /* return immediately after generating work at this depth */
	      AMT_WORK++;
	      work_queue_add(wq, col, left, right, row, n, -1);
	      return 0;
	} else {
		int count = 0;
		    // Call nq_work recursively with appropriate arguments
		unsigned int block = col | left | right;
		while (block + 1 < (1 << n)) {
		  unsigned int open = (block + 1) & (~block);
		  count += nq_work(wq,
				  col | open,
				  ((left | open) << 1) & ((1 << n) - 1),
				  (right | open) >> 1,
				  row + 1,
				  n,
				  parallel_depth);
		  block |= open;
		}
		return count;
	}
}


/* Function for worker thread */
void * nq_worker(void * wq_) {
	work_queue_t wq = (work_queue_t)wq_;
	while (1) { //Go on removing the work from queue and assign it to a worker thread. We will stop once queue becomes empty
	      nqueen_arg_t arg = 0;
	      /* try to fetch work */

	      //Critical Section
	      pthread_mutex_lock(&wq->mx);
	      {
		    int head=  wq-> head;
		    if(head < wq->size) {
			  arg = wq->args + head;
			  wq->head = head + 1;
		    }
	      }
	      pthread_mutex_unlock(&wq->mx);
	      if (arg ==0) break;		/* no work left, so just quit */

	      //Call nq_work recursively and update the value of the count accordingly
	      arg->count = nq_work(wq,arg->col,arg->left_dia,arg->right_dia,arg->row,arg->n,arg->parallel_depth);
	}
}


/**** Master thread that spawns the worker threads and collects the results ****/
int nq_master(int n, int parallel_depth) {
	work_queue_t wq = mk_work_queue();
	/* generate work */
	int count = nq_work(wq, 0, 0, 0, 0, n, parallel_depth);
	int i;

	/* create pthreads that do the generated work */
	pthread_t * thids = (pthread_t *)malloc(sizeof(pthread_t) * N_THREADS);
	for (i = 0; i < N_THREADS; i++) {
	      Create Worker Threads (thread function nq_worker())
	      pthread_create(&thids[i],NULL,nq_worker, (void*) wq);
	}
	for (i = 0; i < N_THREADS; i++) {
	      Endure Thread termination
	      pthread_join(thids[i],NULL);
	}
	for (i = 0; i < wq->size; i++) {
	      //Collect Thread return values from wq
	      count += wq->args[i].count;
	}

	//Free the queue as well
	free(wq->args);
	free(wq);

	return count;
}


void usage(){
	printf ("************* USAGE ***********\n\n");
	printf ("\t ./nqueen <APPROACH> <N> [DEPTH] [NO_OF_THREADS]\n");
	printf ("\t APPROACH = 0 => Serial\n");
	printf ("\t APPROACH = 1 => Threads\n");
	printf ("\t APPROACH = 2 => Threads with Work Pool\n");
	printf ("\t N = Number of queens\n");
	printf ("\t DEPTH = Recursion depth for Threads and Threads with Work approach\n");
	printf ("\t NO_OF_THREADS = Number of worker Threads. Default will be 2.\n");
	return;
}


//Utility function returns current time
double cur_time() {
      struct timeval tp[1];
      gettimeofday(tp, 0);
      return tp->tv_sec + tp->tv_usec * 1.0E-6;
}

//Driver function which takes the arguments and accordingly call the respective functions.
int main(int argc, char * argv[]) {

	if (argc < 3 || argc > 5){
		usage();
		return -1;
	}

	int st = atoi(argv[1]);
	int n = atoi(argv[2]);
	int parallel_depth = -1;

  	if (argc > 3) {
		parallel_depth = atoi(argv[3]);
	}

	if (argc > 4) {
		N_THREADS = atoi(argv[4]);
	}

	double t0 = cur_time();
	int n_solutions = 0;
	int i;
	switch (st){
	case 0:
		printf ("\n\n****** Solving nQueen using serial recursion ******\n\n");
		n_solutions = nq_serial (0, 0, 0, 0, n);
		break;
	case 1:
		printf ("\n\n****** Solving nQueen using threads ******\n\n");
		printf("\n\t No of Queens = %d and Parallel Depth = %d",n,parallel_depth);
		n_solutions = nq_parallel (0, 0, 0, 0, n, parallel_depth);
		printf("\n\tTotal no of Threads created = %d",THREADS);
		THREADS = 0;
		break;
	case 2:
		printf ("\n\n****** Solving nQueen using work pool and threads ******\n\n");
		printf("\n\tNo fo Queens=%, Parallel Depth = %d and No of Threads = %d ",n,parallel_depth,N_THREADS);
		n_solutions = nq_master (n, parallel_depth);
		printf("\n\t Amount of work = %d",AMT_WORK);
		AMT_WORK = 0;
		break;
	default:
		printf ("\n\n****** No Strategy defined for %d ******\n\n", st);
		return -1;
	}
	double t1 = cur_time();
	printf("\n\t%.3f sec (%d queen = %d)\n\n", t1 - t0, n, n_solutions);

	return 0;
}

