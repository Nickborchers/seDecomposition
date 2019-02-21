#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "omp.h"

#define MIN_PIX 0
#define MAX_PIX 255

#define HORIZONTAL 0
#define VERTICAL 1

#define MAX_THREAD_NUM 4

#define DEFAULT_STRIDE 0
#define DEFAULT_CHANNELS 1

#define MAX(a,b)  (((a)>(b)) ? (a):(b))
#define MIN(a,b)  (((a)<(b)) ? (a):(b))

#define RGB_RED 0.299
#define RGB_GREEN 0.587
#define RGB_BLUE 0.114

#define VERBOSE 0

/*
 * Function:  createImage 
 * --------------------
 *  allocates and initializes a new Image object
 *
 *  data: a pointer to the block of pixel data
 *  width: the width of the image
 *  height: the height of the image
 *	channels: the amount of channels for the image (specified in stbi_image_write.h, from line 78)
 * 
 *  returns: a pointer to the new Image object
 */

Image *createImage(Pixel *data, int width, int height, int channels){
  Image *new = malloc(sizeof(struct Image));
  assert(new != NULL);
  new->width = width;
  new->height = height;
  new->channels = channels;
  new->stride = DEFAULT_STRIDE;
  new->data = data;
  return new;
}

/*
 * Function:  readImage 
 * --------------------
 *  reads a new image from the file system
 *
 *  str: the name of the image to be read, it has to be a .png file
 *
 *  returns: a pointer to the new Image object
 */

Image *readImage(char *str){
  int width, height, channels;
  Pixel *data = stbi_load(str,
              &width,
              &height,
              &channels,
              DEFAULT_STRIDE);

  if( data == NULL ) {
    fprintf(stderr, "Reading of image failed, program will now exit.\n");
    exit(-1);
  }

  return createImage(data, width, height, channels);
}

/*
 * Function:  copyImage 
 * --------------------
 *  copies the pixeldata and metadata of an image to a new Image object
 *
 *  im: a pointer to the image to be copied
 *
 *  returns: a pointer to the new Image object
 */

Image *copyImage(Image *im){
  unsigned int i;
  Pixel *data = malloc(im->width * im->height * im->channels * sizeof(struct Image));
  assert(data != NULL);
  Image *cpy = createImage(data, im->width, im->height, im->channels);
  for(i = 0; i < im->width * im->height * im->channels; i++) 
    data[i] = im->data[i];
  return cpy;
}

/*
 * Function:  computeBinaryDiscSE 
 * --------------------
 *  creates a new image object containing a disc-shaped structuring element
 *
 *  radius: the radius of the disc structuring element
 *
 *  returns: a pointer to the new Image object
 */

Image *computeBinaryDiscSE(int radius){
  int i, j, euclideanDistance;
  int size = radius * 2 + 1; // size in 1 dimension. Size = width = height
  int middle = size / 2;
  Pixel *data = calloc(size*size, sizeof(Pixel));
  assert(data != NULL);

  Image *SE = createImage(data, size, size, 1);
  for(i = 0; i < size; i++){
    for(j = 0; j < size; j++){
      euclideanDistance = sqrt( pow(i - middle, 2) + pow(j - middle, 2));
      if( euclideanDistance - radius + 1 < 0 ) SE->data[i * size + j] = MAX_PIX;
    }
  }
  return SE;
}

/*
 * Function:  writeImage 
 * --------------------
 *  writes an image to the file system
 *
 *  str: the name of the image to be read, it has to be a .png file
 *
 */

void writeImage(Image *im, char *name){
  if(!stbi_write_png(name, im->width, im->height, im->channels, (Pixel *) im->data, im->stride)){
    fprintf(stderr, "Writing of image with name: %s failed\n", name);
  }
  freeImage(im);
}

/*
 * Function:  freeImage 
 * --------------------
 *  frees the pixeldata and image metadata
 *
 *  im: a pointer to the image to be freed
 *
 */

void freeImage(Image *im){
  stbi_image_free(im->data);
  free(im);
}

/*
 * Function:  getWidth 
 * --------------------
 *  returns the width of an image 
 *
 *  im: a pointer to the image
 *
 *  returns: the width of the image
 */

int getWidth(Image *im){
  if( im != NULL ) return im->width;
  return 0;
}

/*
 * Function:  getHeight 
 * --------------------
 *  returns the height of an image 
 *
 *  im: a pointer to the image
 *
 *  returns: the height of the image
 */

int getHeight(Image *im){
  if( im != NULL ) return im->height;
  return 0;
}
/*
 * Function:  getChannels 
 * --------------------
 *  returns the channels of an image 
 *
 *  im: a pointer to the image
 *
 *  returns: the channels of the image
 */
int getChannels(Image *im){
  if( im != NULL) return im->channels;
  return 0;
}

/*
 * Function:  getData 
 * --------------------
 *  returns a pointer to the pixeldata of an image
 *
 *  im: a pointer to the image
 *
 *  returns: the pixeldata of the image
 */

Pixel *getData(Image *im){
	if( im != NULL) return im->data;
	return NULL;
}

/*
 * Function:  getPixel 
 * --------------------
 *  returns the value of pixel at row row and column column
 *
 *  im: a pointer to the image
 *  row: the row of the pixel
 *  column: the column of the pixel
 *
 *  returns: the value of the pixel
 */

Pixel getPixel(Image *im, int row, int column){
  return im->data[row*im->height + column];
}

/*
 * Function:  setPixel 
 * --------------------
 *  sets the value of pixel at row row and column column
 *
 *  im: a pointer to the image
 *  row: the row of the pixel
 *  column: the column of the pixel
 *
 */

void setPixel(Image *im, int row, int column, Pixel val){
  im->data[row*im->height + column] = val;
}

/*
 * Function:  dilate 
 * --------------------
 *  computes the dilation of row/column a with a structuring element size s using the HGW algorithm
 * 
 *  a: the pixeldata
 *  n: the size of the pixeldata
 *  c: the 'left' buffer
 *  d: the 'right' buffer 
 *  s: the size of the structuring element
 *
 *  returns: a pointer to the dilated pixeldata
 */

Pixel *dilate(Pixel *a, 
        int n,
        Pixel *c,
        Pixel *d,
        int s
        ){
  int u, i;
  int l = s/2;

  Pixel *b = malloc((n + 1)* sizeof(Pixel));
  assert( b != NULL);
  
  for( u = l; u < n; u += s - 1){

    d[0] = a[u];
    for(i = 1; i < s - 1; i++)
      d[i] = MAX(d[i - 1], a[u + 1]);
    c[s - 2] = a[u - 1];
    for( i = 1; i < s - 1; i++){
      if( u - i - 1 < 0){ //boundary conditions
        c[s - i - 2] = MIN_PIX;
        continue;
      }
      c[s - i - 2] = MAX(c[s - i - 1], a[u - i - 1]);
    }
    for(i = 0; i < s - 1; i++){
      b[u - l + i] = MAX(c[i], d[i]);
    }
  }

  return b;
}


/*
 * Function:  dilate3
 * --------------------
 *  computes the dilation of row/column a with a structuring element size 3 naively,
 *  a more complex algorithm like HGW is unnecessary in this case.
 * 
 *  a: the pixeldata
 *  n: the size of the pixeldata
 *  
 *  returns: a pointer to the eroded pixeldata
 */

Pixel *dilate3(Pixel *a, int n){
  int i;
  Pixel *b = malloc(n * sizeof(Pixel));
  assert( b != NULL);
  
  b[0] = MIN(a[0], a[1]);
  for(i = 1; i < n - 1; i++){
    b[i] = MIN(MIN(a[i - 1], a[i]), a[i + 1]);
  }
  b[n - 1] = MIN(a[n - 2], a[n - 1]);
  return b;
}

/*
 * Function: dilation 
 * --------------------
 *  computes the dilation of an image im
 * 
 *  im: the image to be dilated
 *  s: the size of the structuring element in direction direction
 *  direction: in what direction to treat the pixeldata (HORIZONTAL/VERTICAL)
 *
 */

void dilation(Image *im,
        int s,
        int direction){

  int n = im->width;
  int height = im->height;
  Pixel *a = im->data;
  int row, pix;
  int chunk = height/MAX_THREAD_NUM;
  #pragma omp parallel num_threads(MAX_THREAD_NUM) default(none) private(pix, row) firstprivate(s, chunk, n, height, direction) shared(im, a)
  {
    Pixel *b, *c, *d, *result;
    for( row = chunk * omp_get_thread_num(); row < chunk * (omp_get_thread_num() + 1); row++){      
      b = malloc((n + 1) * sizeof(Pixel)); // buffer
      c = malloc( (s - 1) * sizeof(Pixel)); // left max array
      d = malloc( (s - 1) * sizeof(Pixel)); // right max array
      assert(b != NULL);
      assert(c != NULL);
      assert(d != NULL);

      /* Fill buffer with data */
      #pragma omp critical
      { 
        if( direction == HORIZONTAL ){
          for(pix = 0; pix < n; pix++) {
            b[pix] = a[row*n + pix];
          }
        }else{
          for(pix = 0; pix < n; pix++) {
            b[pix] = a[pix*n + row];
          }
        }
      }
      if( s == 3 ){
        result = dilate3(b, n);
      }else{
        result = dilate(b, n, c, d, s);
        free(c);
        free(d);
      }
      free(b);

      /* Write back buffer to original array */
      #pragma omp critical
      { 
        if( direction == HORIZONTAL ){
          for(pix = 0; pix < n; pix++) {
            a[row*n + pix] = result[pix];
          }
        }else{
          for(pix = 0; pix < n; pix++) {
            a[pix*n + row] = result[pix];
          }
        }
      free(result);
      }
    }
  }
}

/*
 * Function:  erode 
 * --------------------
 *  computes the erosion of row/column a with a structuring element size s using the HGW algorithm
 * 
 *  a: the pixeldata
 *  n: the size of the pixeldata
 *  c: the 'left' buffer
 *  d: the 'right' buffer 
 *  s: the size of the structuring element
 *
 *  returns: a pointer to the eroded pixeldata
 */

Pixel *erode(Pixel *a, 
        int n,
        Pixel *c,
        Pixel *d,
        int s){
  int u, i;
  int l = s/2;
  Pixel *b = malloc((n+1) * sizeof(Pixel));
  assert( b != NULL);
  
  for( u = l; u < n; u += s - 1){

    d[0] = a[u];
    for(i = 1; i < s - 1; i++){
      d[i] = MIN(d[i - 1], a[u + 1]);
    }
    c[s - 2] = a[u - 1];
    for( i = 1; i < s - 1; i++){
      if( u - i - 1 < 0){ //boundary conditions
        c[s - i - 2] = MAX_PIX;
        continue;
      }
      c[s - i - 2] = MIN(c[s - i - 1], a[u - i - 1]);
    }
    for(i = 0; i < s - 1; i++){
      b[u - l + i] = MIN(c[i], d[i]);
    }
  }
  return b;
}


/*
 * Function:  erode3
 * --------------------
 *  computes the erosion of row/column a with a structuring element of size 3 naively,
 *  a more complex algorithm like HGW is unnecessary in this case.
 * 
 *  a: the pixeldata
 *  n: the size of the pixeldata
 *  
 *  returns: a pointer to the eroded pixeldata
 */

Pixel *erode3(Pixel *a, int n){
  int i;
  Pixel *b = malloc(n * sizeof(Pixel));
  assert( b != NULL);
  
  b[0] = MIN(a[0], a[1]);
  for(i = 1; i < n - 1; i++){
    b[i] = MIN(MIN(a[i - 1], a[i]), a[i + 1]);
  }
  b[n - 1] = MIN(a[n - 2], a[n - 1]);
  return b;
}

/*
 * Function: erosion 
 * --------------------
 *  computes the erosion of an image im
 * 
 *  im: the image to be dilated
 *  s: the size of the structuring element in direction direction
 *  direction: in what direction to treat the pixeldata (HORIZONTAL/VERTICAL)
 *
 */

void erosion(Image *im,
      int s,
      int direction){

  int n = im->width;
  int height = im->height;
  Pixel *a = im->data;

  int chunk = height/MAX_THREAD_NUM;
  #pragma omp parallel num_threads(MAX_THREAD_NUM) default(none) firstprivate(s, chunk, n, height, direction) shared(im, a)
  {
    Pixel *b, *c, *d, *result;
    int pix, row;
    for( row = chunk * omp_get_thread_num(); row < chunk * (omp_get_thread_num() + 1); row++){      
      b = malloc((n + 1)* sizeof(Pixel)); // buffer
      c = malloc( (s - 1) * sizeof(Pixel)); // left max array
      d = malloc( (s - 1) * sizeof(Pixel)); // right max array
      assert(b != NULL);
      assert(c != NULL);
      assert(d != NULL);

      /* Fill buffer with data from original array */
      #pragma omp critical
      { 
        if( direction == HORIZONTAL ){
          for(pix = 0; pix < n; pix++) {
            b[pix] = a[row*n + pix];
          }
        }else{
          for(pix = 0; pix < n; pix++) {
            b[pix] = a[pix*n + row];
          }
        }
      }
      if( s == 3){
        result = erode3(b, n);
      }else{
        result = erode(b, n, c, d, s);
        free(c);
        free(d);
      }
      free(b);
      /* Write back buffer to original array */
      #pragma omp critical
      { 
        if( direction == HORIZONTAL ){
          for(pix = 0; pix < n; pix++) {
            a[row*n + pix] = result[pix];
          }
        }else{
          for(pix = 0; pix < n; pix++) {
            a[pix*n + row] = result[pix];
          }
        }
        free(result);
      }
    }
  }
}

/*
 * Function: morphOpening 
 * --------------------
 *  takes an image and a structuring element size and computes the morphological opening of 
 *  that image with a structuring element of dimensions: horizontalSize * verticalSize
 * 
 *  im: the image to be morph opened
 *  p: the partition consisting of a cubic and a sparse factor
 * 
 */

void morphOpening(Image *im, Partition p){
  if( p.cubicFactor.width > 1 )
    erosion(im, p.cubicFactor.width, HORIZONTAL);
  if( p.cubicFactor.height > 1 )  
    erosion(im, p.cubicFactor.height, VERTICAL);
  erodeNaive(im, p.sparseFactor);
  if( p.cubicFactor.width > 1)
    dilation(im, p.cubicFactor.width, HORIZONTAL);
  if( p.cubicFactor.height > 1)
    dilation(im, p.cubicFactor.height, VERTICAL);
  dilateNaive(im, p.sparseFactor);
}

/*
 * Function: morphClosing 
 * --------------------
 *  takes an image and a structuring element size and computes the morphological opening of 
 *  that image with a structuring element of dimensions: horizontalSize * verticalSize
 * 
 *  im: the image to be morph closed
 *  p: the partition consisting of a cubic and a sparse factor
 * 
 */

void morphClosing(Image *im,  Partition p){
  if( p.cubicFactor.width > 1 )
    dilation(im, p.cubicFactor.width, HORIZONTAL);
  if( p.cubicFactor.height > 1 )  
  dilation(im, p.cubicFactor.height, VERTICAL);
  dilateNaive(im, p.sparseFactor);
  if( p.cubicFactor.width > 1 )
    erosion(im, p.cubicFactor.width, HORIZONTAL);
  if( p.cubicFactor.height > 1 )
    erosion(im, p.cubicFactor.height, VERTICAL);
  erodeNaive(im, p.sparseFactor);
}

/*
 * Function: grayscaleToBinary 
 * --------------------
 *  converts a grayscale image to a binary one according to a threshold
 * 
 *  threshold: the threshold to compare the pixel value with
 * 
 */

void grayscaleToBinary(Image *im, int threshold){
  unsigned int i = 0;
  for(i = 0; i < im->width * im->height * im->channels; i++)  
  	im->data[i] = (im->data[i] < threshold ) ? MIN_PIX : MAX_PIX;
}

/*
 * Function: printBinaryImage 
 * --------------------
 *  prints a binary image to stdout
 * 
 *  im: the image to be printed
 * 
 */

void printBinaryImage(Image *im){
  unsigned int i = 0;
  for( i = 0; i < im->width * im->height * im->channels; i++){
    if( i % im->width == 0 ) printf("\n");
    if( im->data[i] == MIN_PIX ) {
      printf(".");
    }else{
      printf("*");
    }
  }
  printf("\n");
}

/*
 * Function: imageUnion 
 * --------------------
 *  
 *  computes the union of an array of images
 * 
 *  ims: a pointer to a block of images, size has to be >= 2
 *  len: the length of the block of images, has to be >= 2
 * 
 */

void imageUnion(Image* ims, int len){
  unsigned int i, pix;
  unsigned int size = ims[0].width * ims[0].height;
  for(i = 1; i < len; i++){
    for(pix = 0; pix < size; pix++) 
    	ims[0].data[pix] = MAX(ims[0].data[pix], ims[i].data[pix]);
  }
}

/*
 * Function: imageBinaryUnion 
 * --------------------
 *  returns the union of images im1 and im2, they are assumed to be of equal
 *  size
 * 
 *  im1: the first image
 *  im2: the second image
 * 
 */

void imageBinaryUnion(Image* im1, Image *im2){
  unsigned int pix;
  unsigned int size = im1->width * im1->height;
  for(pix = 0; pix < size; pix++) 
  	im1->data[pix] = MAX(im1->data[pix], im2->data[pix]);
}

/*
 * Function: imageIntersection 
 * --------------------
 *  
 *  computes the intersection of an array of images
 * 
 *  ims: a pointer to a block of images, size has to be >= 2
 *  len: the length of the block of images, has to be >= 2
 * 
 */

void imageIntersection(Image* ims, int len){
  unsigned int i, pix;
  unsigned int size = ims[0].width * ims[0].height;
  for(i = 1; i < len; i++){
    for(pix = 0; pix < size; pix++) 
    	ims[0].data[pix] = MIN(ims[0].data[pix], ims[i].data[pix]);
  }
}

/*
 * Function: rgbToGrayscale 
 * --------------------
 *  
 *  converts an rgb image to grayscale
 * 
 *  im: a pointer to the image
 * 
 */

void rgbToGrayscale(Image *im){
  unsigned int pixel;
  Pixel *newData = calloc(im->width * im->height, sizeof(unsigned char));
  for(pixel = 0; pixel < im->width * im->height * im->channels; pixel += 3){
    newData[pixel/3] = (int) (RGB_RED * im->data[pixel] 
    													+ RGB_GREEN *im->data[pixel + 1] 
    													+ RGB_BLUE * im->data[pixel + 2]);
  }
  free(im->data);
  im->channels = 1;
  im->data = newData;
}

/*
 * Function: rgbaToGrayscale 
 * --------------------
 *  
 *  converts an rgba image to grayscale
 * 
 *  im: a pointer to the image
 * 
 */

void rgbaToGrayscale(Image *im){
  unsigned int pixel;
  Pixel *newData = calloc(im->width * im->height, sizeof(Pixel));
  for(pixel = 0; pixel < im->width * im->height * im->channels; pixel += 4){
    newData[pixel/4] = (int) (RGB_RED * im->data[pixel] 
    													+ RGB_GREEN *im->data[pixel + 1] 
    													+ RGB_BLUE * im->data[pixel + 2]);
    if(im->data[pixel+3] == 0) newData[pixel/4] = MAX_PIX;
  }
  free(im->data);
  im->channels = 1;
  im->data = newData;
}

/*
 * Function: newQueue 
 * --------------------
 *  
 *  Allocates and initializes a new queue
 * 
 *  returns: a pointer to the new queue
 */

Queue *newQueue(){
  Queue *new;
  new = malloc(sizeof(struct Queue));
  assert(new != NULL);
  new->head = NULL;
  new->tail = NULL;
  new->size = 0;
  return new;
}

/*
 * Function: queueSize 
 * --------------------
 *  
 *  returns the size of Queue qp 
 * 
 *  qp: a pointer to the queue
 * 
 */

int queueSize(Queue *qp){
	if(qp != NULL) return qp->size;
	return -1;
}

/*
 * Function: freeQueue 
 * --------------------
 *  
 *  frees the queue qp
 * 
 *  qp: a pointer to the queue
 * 
 */

void freeQueue(Queue *qp){
	free(qp);
}

/*
 * Function: enqueue 
 * --------------------
 *  
 *  enqueues a partition new in queue qp 
 * 
 *  qp: a pointer to the queue
 *  new: the new partition to be inserted
 * 
 */

void enqueue(Queue *qp, Partition *new){
  if( qp->size == 0 ){
    qp->size = 1;
    qp->head = new;
    qp->tail = new;
  }else{ /* qp->size > 0 */
    qp->tail->next = new;
    qp->tail = new;
    qp->size++;
  }
}

/*
 * Function: dequeue 
 * --------------------
 *  
 *  dequeues a partition from queue qp 
 * 
 *  qp: a pointer to the queue
 *
 *  returns: a pointer to the dequeued partition 
 */

Partition *dequeue(Queue *qp){ 
  if( qp->size < 2) {
    qp->size--;
    return qp->head;
  }

  Partition *n = qp->head;
  qp->head = qp->head->next;
  qp->size--;
  if( qp->size == 1) qp->tail = qp->head;

  return n;
}

int isEmptyQueue(Queue *qp){
  if(qp != NULL) return qp->size < 1;
  return -1;
}

/*
 * Function: smallestMorphOpening
 * --------------------
 *  
 *  finds the smallest cube that SE can be morphologically opened without 
 *  change
 * 
 *  SE: the structuring element to be decomposed
 * 
 *  returns: a partition containing the cubic and sparse factor
 */

Partition *smallestMorphOpening(Image *SE){
  Pixel *data = SE->data;
  unsigned int pix;
  int width = SE->width;
  int height = SE->height;
  int seSize = width * height;
  RunLength top, bottom, left, right;

  int foundRunlength = 0;
  for(pix = 0; pix < seSize / 2; pix++ ){ // loop horizontally over SE
    if( !foundRunlength && (data[pix] == MAX_PIX) ) { //Start counting line
      top.start.row = pix / width;
      top.start.col = pix % height;
      foundRunlength++;
    }
    if( foundRunlength && data[pix] == MIN_PIX ) {
      top.finish.row = pix / width;
      top.finish.col = pix % height;
      break;
    }
  }

  foundRunlength = 0;
  for(pix = 0; pix < seSize; pix += width ){ // loop vertically over SE
    if( !foundRunlength && (data[pix] == MAX_PIX) ) { //Start counting line
      left.start.row = pix / width;
      left.start.col = pix % height;
      foundRunlength++;
    }
    if( foundRunlength && data[pix] == MIN_PIX ) {
      left.finish.row = pix / width;
      left.finish.col = pix % height;
      break;
    }
    if(pix / ((width - 1) * height) > 0)
      pix = pix % width + 1 - width;
  }

  foundRunlength = 0;
  for(pix = seSize-1; pix > 0; pix-- ){ // loop vertically over SE
    if( !foundRunlength && (data[pix] == MAX_PIX) ) { //Start counting line
      bottom.start.row = pix / width;
      bottom.start.col = pix % height;
      foundRunlength++;
    }
    if( foundRunlength && data[pix] == MIN_PIX ) {
      bottom.finish.row = pix / width;
      bottom.finish.col = pix % height;
      break;
    }
  }

  foundRunlength = 0;
  for(pix = seSize-1; pix > 0; pix -= width){ // loop vertically over SE
    if( !foundRunlength && (data[pix] == MAX_PIX) ) { //Start counting line
      right.finish.row = pix / width;
      right.finish.col = pix % height;
      foundRunlength++;
    }
    if( foundRunlength && data[pix] == MIN_PIX ) {
      right.start.row = pix / width;
      right.start.col = pix % height;
      break;
    }
    if(pix <= width)
      pix = seSize - width + pix - 1;
  }

  int topLen = top.finish.col - top.start.col;
  int botLen = bottom.start.col - bottom.finish.col;
  int leftLen = left.finish.row - left.start.row;
  int rightLen = right.finish.row - right.start.row;

  if( topLen != botLen && leftLen != rightLen ){
    fprintf(stderr, "Non-symmetrical SE\n");
    return NULL;
  }

  CubicFactor c;
  c.width = topLen;
  c.height = botLen;

  Partition *p = malloc(sizeof(struct Partition));
  assert( p != NULL );
  p->cubicFactor = c;

  int yOffset, xOffset;
  //compute distance from top/bottom to middle, we know the SE is symmetrical
  for( int i = width/2; i < seSize/2; i += width){
  	if( data[i] == MAX_PIX){
  		yOffset = (width * height/2 - i ) / width;
  		break;
  	}
  }
  // Compute distance from left/right to midde, we know the SE is symmetrical
  for( int i = width * height / 2 - width/2; i < seSize; i++){
  	if( data[i] == MAX_PIX){
  		xOffset = (width * height / 2) - i;
  		break;
  	}
  }

  //compute sparse factor 
  SparseFactor s;
  s.topOffset = yOffset;
  s.bottomOffset = yOffset;
  s.leftOffset = xOffset;
  s.rightOffset = xOffset;
  p->sparseFactor = s;
  return p;
}

/*
 * Function: removePartition 
 * --------------------
 *  
 *  removes the previously computed partition from image im
 * 
 *  im: the image from which the partition should be removed
 * 
 */

void removePartition(Image *im){
  Pixel *data = im->data;
  unsigned int pix;
  int width = im->width;
  int height = im->height;
  int seSize = width * height;

  int foundRunlength = 0;
  for(pix = 0; pix < seSize / 2; pix++ ){ // loop horizontally over SE
    if( foundRunlength && data[pix] == MIN_PIX ) {
      break;
    }
    if(data[pix] == MAX_PIX ) { 
      data[pix] = MIN_PIX;
      foundRunlength++;
    }
  }

  foundRunlength = 0;
  for(pix = 0; pix < seSize; pix += width ){ // loop horizontally over SE
    if( foundRunlength && data[pix] == MIN_PIX ) break;
    if( data[pix] == MAX_PIX ) { //Start counting line
      data[pix] = MIN_PIX;
      foundRunlength++;
    }

    if(pix / ((width - 1) * height) > 0) pix = pix % width + 1 - width;
  }

  foundRunlength = 0;
  for(pix = seSize-1; pix > 0; pix-- ){ // loop horizontally over SE
    if( foundRunlength && data[pix] == MIN_PIX ) break;
    if( data[pix] == MAX_PIX ) {
      data[pix] = MIN_PIX;
      foundRunlength++;
    }
  }

  foundRunlength = 0;
  for(pix = seSize-1; pix > 0; pix -= width){ // loop horizontally over SE
    if( foundRunlength && data[pix] == MIN_PIX ) break;
    if( data[pix] == MAX_PIX ){
      data[pix] = MIN_PIX;
      foundRunlength++;
    }
    if(pix <= width) pix = seSize - width + pix - 1;
  }
}

/*
 * Function: dilateNaive 
 * --------------------
 *  
 *  dilates an image with sparse factor s using a naive approach
 * 
 *  im: the image to be dilated
 *  s: the sparsefactor
 * 
 */

void dilateNaive(Image *im, SparseFactor s){
  int size = im->width * im->height;
  int width = im->width;
  Pixel *newData = malloc(im->width * im->height * sizeof(Pixel));
  assert(newData != NULL);
  int i, max;
  for(i = 0; i < size; i++ ){
    max = MIN_PIX;
    if( i - s.topOffset * width >= 0 )
    	max = MAX(im->data[i - s.topOffset * width], max);
    if( i + s.bottomOffset * width < size )
      max = MAX(im->data[i + s.bottomOffset * width], max);
    if( i - s.leftOffset >= 0 )
    	max = MAX(im->data[i - s.leftOffset], max);
    if( i + s.rightOffset < size )
    	max = MAX(im->data[i + s.rightOffset], max);
    newData[i] = max;
  }
  free(im->data);
  im->data = newData;
}

/*
 * Function: erodeNaive 
 * --------------------
 *  
 *  erodes an image with sparse factor s using a naive approach
 * 
 *  im: the image to be eroded
 *  s: the sparsefactor
 * 
 */

void erodeNaive(Image *im, SparseFactor s){
  int size = im->width * im->height;
  int width = im->width;
  Pixel *newData = malloc(im->width * im->height * sizeof(Pixel));
  assert(newData != NULL);
  int min, i;
  for(i = 0; i < size; i++ ){
    min = MAX_PIX;
    if( i - s.topOffset * width >= 0 ) 
      min = MIN(im->data[i - s.topOffset * width], min);
    if( i + s.bottomOffset * width < size ) 
      min = MIN(im->data[i + s.bottomOffset * width], min);
    if( i - s.leftOffset >= 0 ) 
      min = MIN(im->data[i - s.leftOffset], min);
    if( i + s.rightOffset < size ) 
      min = MIN(im->data[i + s.rightOffset], min);
    newData[i] = min;
  }
  free(im->data);
  im->data = newData;
}

/*
 * Function: decompose 
 * --------------------
 *  
 *  Decomposes a structuring element SE into a union of partitions and 
 *    enqueues each partition
 * 
 *  SE: the structuring element to be decomposed
 *  qp: the queue in which the partitions are stored
 * 
 */

void decompose(Image *SE, Queue *qp){
  Partition *p;
  do{
    p = smallestMorphOpening(SE);
    if( p == NULL ) {
      fprintf(stderr, "Something went wrong while partitioning\n");
      exit(-1);
    }
    enqueue(qp, p);
    removePartition(SE);
    if(VERBOSE) printBinaryImage(SE);
    if(p->sparseFactor.topOffset <= 1 && p->sparseFactor.leftOffset <= 1 ) break;
  }while( p != NULL);
}


