#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "image.c"

#define SE_RADIUS 9
#define GRAYSCALE_TO_BINARY_THRESHOLD 100

/*  
 *  ----------------
 *  Example use of the image.c library: 
 *  run as ./sedecomp.out yourimagename.png 
 *
 */

int main(int argc, char *argv[]){
  if( argc <= 1 ) {
    printf("Image name not provided in command line, program will now exit.\n");
    return 0;
  }

  char *name = argv[1];

  Image *im = readImage(name);

  Image *CSE;
  Partition *p;
  Queue *qp = newQueue();
  CSE = computeBinaryDiscSE(SE_RADIUS);
  printf("Initial SE: \n");
  printBinaryImage(CSE);
  decompose(CSE, qp);
  freeImage(CSE);
  
  while(queueSize(qp) > 0 ) {
    p = dequeue(qp);
    morphOpening(im, *p);
    free(p);
  }
  // writeImage(im, "closing.png");
  freeImage(im);
  freeQueue(qp);
  return 0;
}
