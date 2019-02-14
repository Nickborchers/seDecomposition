#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "image.c"

#define SE_SIZE 5
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
    p = smallestMorphClosing(CSE);
    if( p == NULL ) {
      fprintf(stderr, "Something went wrong while partitioning\n");
      exit(-1);
    }
    setImage(p, copyImage(im));
    enqueue(qp, p);
    removePartition(CSE);
    printBinaryImage(CSE);
    if(p->sparseFactor.topOffset ==  1 && p->sparseFactor.leftOffset == 1 ) break;
  }while( p != NULL);

  fprintf(stderr, "queuesize: %d\n", qp->size);

  ImageQueue *iqp;
  iqp = newImageQueue();
  while( queueSize(qp) ){
    p = dequeue(qp);
    dilation(p->im, NULL, p->cubicFactor.width, HORIZONTAL); //We use van Herk/Gil-Werman for the cubic factors
    dilation(p->im, NULL, p->cubicFactor.height, VERTICAL);
    imageQueueEnqueue(iqp, p->im);
    // dilateNaive(im, p->sparseFactor);
    free(p);
  }

  Node *n;
  while( imageQueueSize(iqp) > 0){
    n = imageQueueDequeue(iqp);
    dilation(p->im, NULL, p->cubicFactor.width, HORIZONTAL); //We use van Herk/Gil-Werman for the cubic factors
    dilation(p->im, NULL, p->cubicFactor.height, VERTICAL);
    imageQueueEnqueue(iqp, p->im);
    // dilateNaive(im, p->sparseFactor);
    free(p);
  }

  freeImage(CSE);
  freeQueue(qp);
  writeImage(im, "output.png");
  return 0;
}
