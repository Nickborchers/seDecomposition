#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "image.c"

#define SE_SIZE 27
#define GRAYSCALE_TO_BINARY_THRESHOLD 100

// STBI_grey       = 1
// STBI_grey_alpha = 2
// STBI_rgb        = 3
// STBI_rgb_alpha  = 4


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

  CSE = computeBinaryDiscSE(SE_SIZE);
  printBinaryImage(CSE);
  do{
    p = smallestMorphOpening(CSE);
    if( p == NULL ) {
      fprintf(stderr, "Something went wrong while partitioning\n");
      exit(-1);
    }
    enqueue(qp, p);
    removePartition(CSE);
    printBinaryImage(CSE);
    if(p->sparseFactor.topOffset ==  1 && p->sparseFactor.leftOffset == 1 ) break;
  }while( p != NULL);

  fprintf(stderr, "queuesize: %d\n", qp->size);
  return 0;
}
