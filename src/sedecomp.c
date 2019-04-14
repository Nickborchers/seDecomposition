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

  
  // printf("Initial SE: \n");
  // printBinaryImage(CSE);
  clock_t begin = clock();
  decompose(CSE, qp);
  clock_t end = clock();
  double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  fprintf(stderr, "Decomposition done: time spent: %lf\n", time_spent);
  begin = clock();
  int counter = 0;
  while(queueSize(qp) > 0 ) {
    p = dequeue(qp);
    morphOpening(opening, *p);
    morphClosing(closing, *p);
    free(p);
  }

  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  fprintf(stderr, "All computing done: time spent: %lf\n", time_spent);
  char fileNameOpened[FILENAME_BUFFER_SIZE] = "morph_opened_";
  strcat(fileNameOpened, name);
  writeImage(opening, fileNameOpened);

  freeImage(CSE);
  freeQueue(qp);
  return 0;
}
