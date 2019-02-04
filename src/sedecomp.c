#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "image.c"

#define SE_SIZE 11
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

  Image *SE, *CSE;
  Partition *p;
  Queue *qp = newQueue();

  CSE = computeBinaryDiscSE(SE_SIZE);
  printBinaryImage(CSE);
  do{
    p = smallestMorphClosing(CSE);
    enqueue(qp, p);
    removePartition(CSE);
    fprintf(stderr, "topOffset: %d\n", p->sparseFactor.topOffset);
    printBinaryImage(CSE);
  }while( p != NULL);

  Partition *partition;
  while(!isEmptyQueue(qp)){
    partition = dequeue(qp);
    fprintf(stderr, "queuesize: %d\n", qp->size);
    dilation(im, NULL, partition->cubicFactor.width, HORIZONTAL);
    dilation(im, NULL, partition->cubicFactor.height, VERTICAL);
    dilateNaive(im, partition->sparseFactor);
  }
  
  writeImage(im->data, "output.png");
  return 0;
}
