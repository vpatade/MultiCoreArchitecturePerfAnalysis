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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#pragma warning (disable: 4244)
#pragma warning (disable: 4305)
#endif

#include <pthread.h>
#include <stdio.h>
#include <algorithm>
#include "../timing.h"
#include "mandelbrot_ispc.h"
using namespace ispc;

//Global Variables
unsigned int width;                                                           
unsigned int height;                                                          
float x0;                                                                      
float x1;                                                                       
float y0;                                                                      
float y1; 
int maxIterations;                                                           
int *buf;
int NUMTHREAD = 2;

//Extern function from mandelbrot_serial.c
extern void mandelbrot_serial(float x0, float y0, float x1, float y1,
		int width, int height, int maxIterations,
		int output[]);


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
	printf("Wrote image file %s\n", fn);
}


int main() {
	
	    width = 768;                                                           
	    height = 512;                                                          
	    x0 = -2;                                                                      
	    x1 = 1;                                                                       
	    y0 = -1;                                                                      
	    y1 = 1;                                                                      
	    maxIterations = 256;                                                            
	    buf = new int[width*height]; 

	    //
	    // Compute the image using the ispc implementation; report the minimum
	    // time of three runs.
	    //
	    double minISPC = 1e30;
	    for (int i = 0; i < 3; ++i) {
		reset_and_start_timer();
		mandelbrot_ispc(x0, y0, x1, y1, width, height, maxIterations, buf);
		double dt = get_elapsed_mcycles();
		minISPC = std::min(minISPC, dt);
	    }

	    printf("[mandelbrot ispc]:\t\t[%.3f] million cycles\n", minISPC);
	    writePPM(buf, width, height, "mandelbrot-ispc.ppm");

	    // Clear out the buffer
	    for (unsigned int i = 0; i < width * height; ++i)
		    buf[i] = 0;

	    // 
	    // And run the serial implementation 3 times, again reporting the
	    // minimum time.
	    //
	    double minSerial = 1e30;
	    for (int i = 0; i < 3; ++i) {
		    reset_and_start_timer();
		    mandelbrot_serial(x0, y0, x1, y1, width, height, maxIterations, buf);
		    double dt = get_elapsed_mcycles();
		    minSerial = std::min(minSerial, dt);
	    }
	    printf("[mandelbrot serial]:\t\t[%.3f] millon cycles\n", minSerial);
	    writePPM(buf, width, height, "mandelbrot-serial.ppm");
	    
	    //Print the speed up
	    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial/minISPC);
      
	    return 0;
}