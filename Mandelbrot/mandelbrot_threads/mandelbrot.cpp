/*
   Copyright (c) 2010-2011, Intel Corporation
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
met:

 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 * Neither the name of Intel Corporation nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.


 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 */
/**
 Code : Mandelbrot using threads
 Name : Vallabh Patade
 ASU ID : 1206277484
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#pragma warning (disable: 4244)
#pragma warning (disable: 4305)
#endif

#include <pthread.h>
#include <stdio.h>
#include <algorithm>
#include "mandelbrot_serial.cpp"
#include "mandelbrot_threads.cpp"
#include "mandelbrot_ispc.h"
#include <string.h>
using namespace ispc;

int NUMTHREAD = 2;

extern void mandelbrot_serial(float x0, float y0, float x1, float y1,
		int width, int height, int maxIterations,
		int output[]);

extern void mandelbrot_threads(float x0, float y0, float x1, float y1,
		int width, int height, int height_start, int height_end, int maxIterations,
		int output[]);

extern void *print_threads(void* threadid);

/* Write a PPM image file with the image of the Mandelbrot set */
static void
writePPM(int *buf, int width, int height, const char *fn) {
	FILE *fp = fopen(fn, "wb");
	fprintf(fp, "P6\n");
	fprintf(fp, "%d %d\n", width, height);
	fprintf(fp, "255\n");
	for (int i = 0; i < width*height; ++i) {
		// Map the iteration count to colors by just alternating between
		// two greys.
		char c = (buf[i] & 0x1) ? 240 : 20;
		for (int j = 0; j < 3; ++j)
			fputc(c, fp);
	}
	fclose(fp);
	printf("\n\t\tWrote image file %s\n", fn);
}


int main() {

	int width = 768;                                                           
	int height = 512;                                                          
	int x0 = -2;                                                                      
	int x1 = 1;                                                                       
	int y0 = -1;                                                                      
	int y1 = 1;                                                                      
	int maxIterations = 256;                                                            
	int* buf = new int[width*height]; 

 	// 
	// Run the serial implementation 3 times, reporting the minimum time.
	//
	double minSerial = 1e30;
	for (int i = 0; i < 3; ++i) {
		reset_and_start_timer();
		mandelbrot_serial(x0, y0, x1, y1, width,height, maxIterations, buf);
		double dt = get_elapsed_mcycles();
		minSerial = std::min(minSerial, dt);
	}

	printf("[mandelbrot serial]:\t\t[%.3f] millon cycles\n", minSerial);
	writePPM(buf, width, height, "mandelbrot-serial.ppm");

	/* Start of pthread version */
	// Clear out the buffer                                                                                                                               
	for (unsigned int i = 0; i < width * height; ++i)
		buf[i] = 0;
	
	//Create File Names Array
	char** fileNames =  new char*[8];
	for(int i=0;i<8;i++) {
	  fileNames[i] = new char[30];
	}
	strcpy(fileNames[0], "mandelbrot-one-thread.ppm");
	strcpy(fileNames[1],"mandelbrot-two-thread.ppm");
	strcpy(fileNames[2],"mandelbrot-three-thread.ppm");
	strcpy(fileNames[3],"mandelbrot-four-thread.ppm");
	strcpy(fileNames[4],"mandelbrot-five-thread.ppm");
	strcpy(fileNames[5],"mandelbrot-six-thread.ppm");
	strcpy(fileNames[6],"mandelbrot-seven-thread.ppm");
	strcpy(fileNames[7],"mandelbrot-eight-thread.ppm");
	                                                                                                                              
	/** 
	  Will run the program for  1, 2, 3, .....8 threads each time will run the loop thrice and will report the minimum time                                                                                         
	*/
	
	for(int num = 1;num<=8;num++) {
		double minThread = 1e30;
		NUMTHREAD = num;
		printf("\n-----------------------------------------------------------------------------------------------------------------------------------------");
		printf("\n\tNo of Threads = %d",NUMTHREAD);
		for (int i = 1; i <= 3; ++i) {
			printf("\n\t\tRun# %d",i);
			
			//We will be creating no of threads -1 no fo threads as one thread will be the master thread itself.
			pthread_t* t_ids = new pthread_t[NUMTHREAD-1];
			thread_args* args = new thread_args[NUMTHREAD-1];
			int rc;

			//Reset the timer
			reset_and_start_timer();
			
			//Start Height and block height;
			int START_SIZE = 0;
			int BLOCK_SIZE = height / NUMTHREAD;
			
			for(int k=0;k < NUMTHREAD-1;k++) {
			      args[k].width = width;
			      args[k].height = height;
			      args[k].height_start = START_SIZE;
			      args[k].height_end = START_SIZE+BLOCK_SIZE;                                                          
			      args[k].x0 = -2;                                                                      
			      args[k].x1 = 1;                                                                       
			      args[k].y0 = -1;                                                                      
			      args[k].y1 = 1;                                                                      
			      args[k].maxIterations = 256;
			      args[k].buf = buf;
			      
			      START_SIZE+=BLOCK_SIZE;
			      
			      // Loop to spawn NUMTHREAD-1 threads
			      // Spawn threads with appropriate argument setting what partial problem the thread will be working on
			      // HINT: define a struct to pass to print_threads instead of k
			      rc = pthread_create(&t_ids[k], NULL, print_threads, (void *)&args[k]);
			      if (rc)
			      {
				      printf("\n\n\tERROR: Thread Creation FAILED: %d\n", rc);
				      exit(-1);
			      }
			}
			// TODO: Call mandelbrot_serial with the appropriate arguments
			// main() thread will also perfrom useful partial computation	
			mandelbrot_threads(x0, y0, x1, y1, width, height, START_SIZE,height, maxIterations, buf);
			// Guarantee target thread(s) termination
			for(int k=0;k<NUMTHREAD-1;k++) {
			    rc = pthread_join(t_ids[k], NULL);
			    if (rc){
				    printf("\n\n\tERROR: Thread Join FAILED: %d\n", rc);
				    exit(-1);
			    }
			}

			double dt = get_elapsed_mcycles();
			minThread = std::min(minThread, dt);
			
			
			
			//Free the memory
			free(t_ids);
			free(args);			
		}

		printf("\n\t\t[mandelbrot thread ] with %d threads :\t\t[%.3f] millon cycles\n", NUMTHREAD, minThread);
		writePPM(buf, width, height, fileNames[NUMTHREAD-1]);//"mandelbrot-thread-two-threads.ppm");

		/* End of pthread version */
		
		// Report speedup over serial mandelbrot
		printf("\n\t\t(%.2fx speedup from threading with %d threads)\n", minSerial/minThread, NUMTHREAD);
	}
	//Free File Names array
	for(int k=0;k<8;k++) {
	    free(fileNames[k]);
	}
	return 0;
}
