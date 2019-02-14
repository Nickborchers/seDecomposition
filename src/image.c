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

#define DEFAULT_STRIDE 0 //works for all PNGs up until now
#define DEFAULT_CHANNELS 1

#define MAX(a,b)  (((a)>(b)) ? (a):(b))
#define MIN(a,b)  (((a)<(b)) ? (a):(b))

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

Image *copyImage(Image *im){
  unsigned int i;
  Pixel *data = malloc(im->width * im->height * im->channels * sizeof(struct Image));
  assert(data != NULL);
  Image *cpy = createImage(data, im->width, im->height, im->channels);
  for(i = 0; i < im->width * im->height * im->channels; i++) 
    data[i] = im->data[i];
  return cpy;
}

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

void writeImage(Image *im, char *name){
  if(!stbi_write_png(name, im->width, im->height, im->channels, (unsigned char *) im->data, im->stride)){
    fprintf(stderr, "Writing of image with name: %s failed\n", name);
  }
  freeImage(im);
}

void freeImage(Image *im){
  stbi_image_free(im->data);
  free(im);
}

int getWidth(Image *im){
  if( im != NULL ) return im->width;
  return 0;
}

int getHeight(Image *im){
  if( im != NULL ) return im->height;
  return 0;
}

int getChannels(Image *im){
  if( im != NULL) return im->channels;
  return 0;
}

Pixel *getData(Image *im){
  return im->data;
}

Pixel getPixel(Image *im, int row, int column){
  return im->data[row*im->height + column];
}

void setPixel(Image *im, int row, int column, Pixel val){
  im->data[row*im->height + column] = val;
}

Pixel *dilate(Pixel *a, 
        int n,
        Pixel *c,
        Pixel *d,
        Pixel *se, 
        int s
        ){
  int u, i;
  int l = s/2;

  Pixel *b = malloc(n * sizeof(Pixel));
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

//negating the existence of channels:
void dilation(Image *im, 
        Pixel *se,
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
      b = malloc(n * sizeof(Pixel)); // buffer
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
      result = dilate(b, n, c, d, NULL, s);
      free(b);
      free(c);
      free(d);


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

Pixel *erode(Pixel *a, 
        int n,
        Pixel *c,
        Pixel *d,
        Pixel *se, 
        int s){
  int u, i;
  int l = s/2;
  Pixel *b = malloc(n * sizeof(Pixel));
  assert( b != NULL);
  
  for( u = l; u < n; u += s - 1){

    d[0] = a[u];
    for(i = 1; i < s - 1; i++)
      d[i] = MIN(d[i - 1], a[u + 1]);
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

void erosion(Image *im, 
      Pixel *se,
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
      b = malloc(n * sizeof(Pixel)); // buffer
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
      result = erode(b, n, c, d, NULL, s);
      free(b);
      free(c);
      free(d);

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
 * morphClosing takes an image and a structuring element size and computes the morphological opening of 
 * that image with a structuring element of dimensions: seSize * seSize
 */

void morphOpening(Image *im, int seSize){
  erosion(im, NULL, seSize, HORIZONTAL);
  erosion(im, NULL, seSize, VERTICAL);
  dilation(im, NULL, seSize, HORIZONTAL);
  dilation(im, NULL, seSize, VERTICAL);
}

/*
 * morphClosing takes an image and a structuring element size and computes the morphological closing of 
 * that image with a structuring element of dimensions: seSize * seSize
 */

void morphClosing(Image *im, int seSize){
  dilation(im, NULL, seSize, HORIZONTAL);
  dilation(im, NULL, seSize, VERTICAL);
  erosion(im, NULL, seSize, HORIZONTAL);
  erosion(im, NULL, seSize, VERTICAL);
}

void grayscaleToBinary(Image *im, int threshold){
  unsigned int i = 0;
  for(i = 0; i < im->width * im->height * im->channels; i++)  im->data[i] = (im->data[i] < threshold ) ? MIN_PIX : MAX_PIX;
}

/*
 * prints a binary image in terminal: if pixel is 0 then print 0 otherwise 1
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
 * imageUnion takes an array of binary images and returns their union
 * they are assumed to be of equal size.
*/

Image* imageUnion(Image* ims, int len){
  unsigned int i, pix;
  unsigned int size = ims[0].width * ims[0].height;
  for(i = 1; i < len; i++){
    for(pix = 0; pix < size; pix++) ims[0].data[pix] = MAX(ims[0].data[pix], ims[i].data[pix]);
  }
  return ims;
}

/* 
 * imageBinaryUnion takes two binary images and returns their union
 * they are assumed to be of equal size.
*/

Image* imageUnion(Image* im1, Image *im2, int len){
  unsigned int i, pix;
  unsigned int size = ims[0].width * ims[0].height;
  for(i = 1; i < len; i++){
    for(pix = 0; pix < size; pix++) ims[0].data[pix] = MAX(ims[0].data[pix], ims[i].data[pix]);
  }
  return ims;
}

/* 
 * imageIntersection takes a pointer of binary images and returns their intersection
 * they are assumed to be of equal size.
*/

Image* imageIntersection(Image* ims, int len){
  unsigned int i, pix;
  unsigned int size = ims[0].width * ims[0].height;
  for(i = 1; i < len; i++){
    for(pix = 0; pix < size; pix++) ims[0].data[pix] = MIN(ims[0].data[pix], ims[i].data[pix]);
  }
  return ims;
}

/*
 * Converts an image with 3 channels to an image with 1 channel
*/

void rgbToGrayscale(Image *im){
  unsigned int pixel;
  Pixel *newData = calloc(im->width * im->height, sizeof(unsigned char));
  for(pixel = 0; pixel < im->width * im->height * im->channels; pixel += 3){
    newData[pixel/3] = (int) (0.299 * im->data[pixel] + 0.587 *im->data[pixel + 1] + 0.114 * im->data[pixel + 2]);
  }
  free(im->data);
  im->channels = 1;
  im->data = newData;
}

/*
 * Converts an image with 3 channels to an image with 1 channel
*/

void rgbaToGrayscale(Image *im){
  unsigned int pixel;
  Pixel *newData = calloc(im->width * im->height, sizeof(Pixel));
  for(pixel = 0; pixel < im->width * im->height * im->channels; pixel += 4){
    newData[pixel/4] = (int) (0.299 * im->data[pixel] + 0.587 *im->data[pixel + 1] + 0.114 * im->data[pixel + 2]);
    if(im->data[pixel+3] == 0) newData[pixel/4] = MAX_PIX;
  }
  free(im->data);
  im->channels = 1;
  im->data = newData;
}

Queue *newQueue(){
  Queue *new;
  new = malloc(sizeof(Queue));
  assert(new != NULL);
  new->head = NULL;
  new->tail = NULL;
  new->size = 0;
  return new;
}

int queueSize(Queue *qp){
	return qp->size;
}

void freeQueue(Queue *qp){
	free(qp);
}

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

void setImage(Partition *p, Image *im){
	p->im = im;
}

void printCoordinate(Coordinate c){
  printf("Coordinate: row: %d, col: %d\n", c.row, c.col);
}

void printRunLength(RunLength r){
  printCoordinate(r.start);
  printCoordinate(r.finish);
}

int isEmptyQueue(Queue *qp){
  return qp-> size < 1;
}

ImageQueue *newImageQueue(){
  ImageQueue *new;
  new = malloc(sizeof(struct ImageQueue));
  assert(new != NULL);
  new->head = NULL;
  new->tail = NULL;
  new->size = 0;
  return new;
}

void imageQueueEnqueue(ImageQueue *qp, Image *im){
	Node *new = malloc(sizeof(struct Node));
	new->image = im;
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

Node *imageQueueDequeue(ImageQueue *qp){
	 if( qp->size < 2) {
    qp->size--;
    return qp->head;
  }

  Node *n = qp->head;
  qp->head = qp->head->next;
  qp->size--;
  if( qp->size == 1) qp->tail = qp->head;

  return n;
}

int imageQueueSize(ImageQueue *qp){
	return qp->size;
}

/* Find smallest morphological closing without change of the current structuring element,
  the result is the combination of the cubic and the sparse factor */

Partition *smallestMorphClosing(Image *SE){
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
  for(pix = 0; pix < seSize; pix += width ){ // loop horizontally over SE
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
    if(pix / ((width - 1) * height) > 0){
      pix = pix % width + 1 - width;
    } 
  }

  foundRunlength = 0;
  for(pix = seSize; pix > 0; pix-- ){ // loop horizontally over SE
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
  for(pix = seSize; pix > 0; pix -= width){ // loop horizontally over SE
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
    if(pix <= width){
      pix = seSize - width + pix - 1;
    } 
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
  		fprintf(stderr, "yOffset: %d\n", yOffset);
  		break;
  	}
  }
  // Compute distance from left/right to midde, we know the SE is symmetrical
  for( int i = width * height / 2 - width/2; i < seSize; i++){
  	if( data[i] == MAX_PIX){
  		xOffset = (width * height / 2) - i;
  		fprintf(stderr, "yOffset: %d\n", xOffset);
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

void removePartition(Image *SE){
  Pixel *data = SE->data;
  unsigned int pix;
  int width = SE->width;
  int height = SE->height;
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
  for(pix = seSize; pix > 0; pix-- ){ // loop horizontally over SE
    if( foundRunlength && data[pix] == MIN_PIX ) break;
    if( data[pix] == MAX_PIX ) {
      data[pix] = MIN_PIX;
      foundRunlength++;
    }
  }

  foundRunlength = 0;
  for(pix = seSize; pix > 0; pix -= width){ // loop horizontally over SE
    if( foundRunlength && data[pix] == MIN_PIX ) break;
    if( data[pix] == MAX_PIX ){
      data[pix] = MIN_PIX;
      foundRunlength++;
    }
    if(pix <= width) pix = seSize - width + pix - 1;
  }
}

void dilateNaive(Image *im, SparseFactor s){
  int size = im->width * im->height;
  int width = im->width;
  int i;
  int max = MIN_PIX;
  for(i = 0; i < size; i++ ){
    if( i - s.topOffset * width < size && i - s.topOffset * width >= 0 ) //bounds checking
    	max = MAX(im->data[i - s.topOffset * width], max);
    if( i + s.bottomOffset * width < size && i + s.bottomOffset * width >= 0 ) 
    	max = MAX(im->data[i + s.bottomOffset * width], max);
    if( i -  s.leftOffset < size && i - s.leftOffset >= 0 ) 
    	max = MAX(im->data[i - s.topOffset], max);
    if( i + s.rightOffset < size && i + s.rightOffset >= 0 ) 
    	max = MAX(im->data[i + s.rightOffset], max);
    im->data[i] = max;
  }
}