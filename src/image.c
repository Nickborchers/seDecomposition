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

Image *createImage(unsigned char *data, int width, int height, int channels){
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
	unsigned char *data = stbi_load(str,
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
	unsigned char *data = malloc(im->width * im->height * im->channels * sizeof(struct Image));
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
	unsigned char *data = calloc(size*size, sizeof(unsigned char));
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
	if(!stbi_write_png(name, im->width, im->height, im->channels, im->data, im->stride)){
		fprintf(stderr, "Writing of image with name: %s failed\n", name);
	}
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

unsigned char *getData(Image *im){
	return im->data;
}

unsigned char min(unsigned char a, unsigned char b){
	if( a < b ) return a;
	return b;
}

unsigned char max(unsigned char a, unsigned char b){
	if( a > b ) return a;
	return b;
}

unsigned char getPixel(Image *im, int row, int column){
	return im->data[row*im->height + column];
}

void setPixel(Image *im, int row, int column, unsigned char val){
	im->data[row*im->height + column] = val;
}

unsigned char *dilate(unsigned char *a, 
				int n,
				unsigned char *c,
				unsigned char *d,
				unsigned char *se, 
				int s
				){
	int u, i;
	int l = s/2;

	unsigned char *b = malloc(n * sizeof(unsigned char));
	assert( b != NULL);
	
	for( u = l; u < n; u += s - 1){

		d[0] = a[u];
		for(i = 1; i < s - 1; i++)
			d[i] = max(d[i - 1], a[u + 1]);
		c[s - 2] = a[u - 1];
		for( i = 1; i < s - 1; i++){
			if( u - i - 1 < 0){ //boundary conditions
				c[s - i - 2] = MIN_PIX;
				continue;
			}
			c[s - i - 2] = max(c[s - i - 1], a[u - i - 1]);
		}
		for(i = 0; i < s - 1; i++){
			b[u - l + i] = max(c[i], d[i]);
		}
	}

	return b;
}

//negating the existence of channels:
void dilation(Image *im, 
				unsigned char *se,
				int s,
				int direction){

	int n = im->width;
	int height = im->height;
	unsigned char *a = im->data;
	int row, pix;

	int chunk = height/MAX_THREAD_NUM;
	#pragma omp parallel num_threads(MAX_THREAD_NUM) default(none) private(pix, row) firstprivate(s, chunk, n, height, direction) shared(im, a)
	{
		unsigned char *b, *c, *d, *result;
		for( row = chunk * omp_get_thread_num(); row < chunk * (omp_get_thread_num() + 1); row++){			
			b = malloc(n * sizeof(unsigned char)); // buffer
			c = malloc( (s - 1) * sizeof(unsigned char)); // left max array
			d = malloc( (s - 1) * sizeof(unsigned char)); // right max array
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

unsigned char *erode(unsigned char *a, 
				int n,
				unsigned char *c,
				unsigned char *d,
				unsigned char *se, 
				int s){
	int u, i;
	int l = s/2;
	unsigned char *b = malloc(n * sizeof(unsigned char));
	assert( b != NULL);
	
	for( u = l; u < n; u += s - 1){

		d[0] = a[u];
		for(i = 1; i < s - 1; i++)
			d[i] = min(d[i - 1], a[u + 1]);
		c[s - 2] = a[u - 1];
		for( i = 1; i < s - 1; i++){
			if( u - i - 1 < 0){ //boundary conditions
				c[s - i - 2] = MAX_PIX;
				continue;
			}
			c[s - i - 2] = min(c[s - i - 1], a[u - i - 1]);
		}
		for(i = 0; i < s - 1; i++){
			b[u - l + i] = min(c[i], d[i]);
		}
	}
	return b;
}

void erosion(Image *im, 
			unsigned char *se,
			int s,
			int direction){

	int n = im->width;
	int height = im->height;
	unsigned char *a = im->data;

	int chunk = height/MAX_THREAD_NUM;

	#pragma omp parallel num_threads(MAX_THREAD_NUM) default(none) firstprivate(s, chunk, n, height, direction) shared(im, a)
	{
		unsigned char *b, *c, *d, *result;
		int pix, row;
		for( row = chunk * omp_get_thread_num(); row < chunk * (omp_get_thread_num() + 1); row++){			
			b = malloc(n * sizeof(unsigned char)); // buffer
			c = malloc( (s - 1) * sizeof(unsigned char)); // left max array
			d = malloc( (s - 1) * sizeof(unsigned char)); // right max array
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
	for(i=0; i<im->width*im->height*im->channels; i++)	im->data[i] = (im->data[i] < threshold ) ? MIN_PIX : MAX_PIX;
}

/*
 * prints a binary image in terminal: if pixel is 0 then print 0 otherwise 1
*/

void printBinaryImage(Image *im){
	unsigned int i = 0;
	for( i = 0; i < im->width * im->height * im->channels; i++){
		if( i % im->width == 0 ) printf("\n");
		if( im->data[i] == MIN_PIX ) {
			printf(" ");
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
		for(pix = 0; pix < size; pix++) ims[0].data[pix] = max(ims[0].data[pix], ims[i].data[pix]);
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
		for(pix = 0; pix < size; pix++) ims[0].data[pix] = min(ims[0].data[pix], ims[i].data[pix]);
	}
	return ims;
}

/*
 * Converts an image with 3 channels to an image with 1 channel
*/

void rgbToGrayscale(Image *im){
	unsigned int pixel;
	unsigned char *newData = calloc(im->width * im->height, sizeof(unsigned char));
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
	unsigned char *newData = calloc(im->width * im->height, sizeof(unsigned char));
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

Node *newNode(CubicFactor c, SparseFactor s){
	Node *new = malloc(sizeof(Node));
	assert(new != NULL);
	new->cubicFactor = c;
	new->sparseFactor = s;
	new->next = NULL;
	return new;
}

void enqueue(Queue *qp, CubicFactor c, SparseFactor s){
	Node *new = newNode(c, s);
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

Node *dequeue(Queue *qp){
	if( qp->size < 2) return qp->head;

	Node *n = qp->head;
	qp->head = qp->head->next;
	qp->size--;
	if( qp->size == 1) qp->tail = qp->head;

	return n;
}

/* Find smallest morphological closing without change of the current structuring element,
	the result is the combination of the cubic and the sparse factor */

Node *smallestMorphClosing(Image *SE){
	unsigned int pix;
	int width = SE->width;
	int height = SE->height;
	int seSize = width * height;
	unsigned char *data = SE->data;
	int cubicFactorTopWidth = 0;
	int cubicFactorBottomWidth = 0;
	int cubicFactorRightHeight = 0;
	int cubicFactorLeftHeight = 0;
	int sparseX, sparseY;

	for(pix = 0; pix < seSize / 2; pix++ ){ // loop horizontally over SE
		if( data[pix] == MAX_PIX ) { //Start counting line
			cubicFactorTopWidth++;
		}
		if( cubicFactorTopWidth && data[pix] == MIN_PIX ) break;
	}

	for(pix = seSize; pix > seSize / 2; pix-- ){ // loop horizontally over SE
		if( data[pix] == MAX_PIX ) { //Start counting line
			cubicFactorBottomWidth++;
		}
		if( cubicFactorBottomWidth && data[pix] == MIN_PIX ) break;
	}


	for(pix = 0; pix < seSize; pix++ ){ // loop vertically over SE
		printf("pix: %d\n", pix * width + pix % height);
		if( data[pix * width + pix % height] == MAX_PIX ) { //Start counting line
			cubicFactorLeftHeight++;
		}
		if( cubicFactorLeftHeight && data[pix * width + pix % height] == MIN_PIX ) break;
	}
	printf("CubicFactorTopWidth: %d, CubicFactorBottomWidth: %d\n, CubicFactorLeftHeight: %d, cubicFactorRightHeight: %d\n", 
		cubicFactorTopWidth, 
		cubicFactorBottomWidth, 
		cubicFactorLeftHeight, 
		cubicFactorRightHeight);
	//Find sparse factor
	return NULL;
}