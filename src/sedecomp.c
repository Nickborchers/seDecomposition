#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "image.c"

#define SE_RADIUS 9
#define GRAYSCALE_TO_BINARY_THRESHOLD 100
#define FILENAME_BUFFER_SIZE 128

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

  int seRadius = SE_RADIUS;
  if( argc >= 3)
    seRadius = atoi(argv[2]);

  char *name = argv[1];

  Image *opening = readImage(name);
  Image *closing = readImage(name);
  Image *CSE;
  Partition *p;
  Queue *qp = newQueue();
  CSE = computeBinaryDiscSE(seRadius);
  
  printf("Initial SE: \n");
  // printBinaryImage(CSE);
  decompose(CSE, qp);

  clock_t begin = clock();
  while(queueSize(qp) > 0 ) {
    p = dequeue(qp);
    morphOpening(opening, *p);
    morphClosing(closing, *p);
    free(p);
  }

  clock_t end = clock();
  double timeExpired = ((double) (end - begin)) / CLOCKS_PER_SEC;
  fprintf(stderr, "Time it took: %lf\n", timeExpired);
  char fileNameOpened[FILENAME_BUFFER_SIZE] = "morph_opened_";
  char fileNameClosed[FILENAME_BUFFER_SIZE] = "morph_closed_";
  strcat(fileNameOpened, name);
  strcat(fileNameClosed, name);
  writeImage(opening, fileNameOpened);
  writeImage(closing, fileNameClosed);
  freeImage(CSE);
  freeQueue(qp);
  return 0;
}
