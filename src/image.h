#ifndef IMAGE
#define IMAGE

typedef unsigned char Pixel;

typedef struct Image {
	int width;
	int height;
	int channels;
	int stride;
	Pixel *data;
}Image;

typedef struct Coordinate{
	int row;
	int col;
}Coordinate;

typedef struct RunLength{
	Coordinate start;
	Coordinate finish;
}RunLength;

typedef struct CubicFactor{
	int width;
	int height;
}CubicFactor;

typedef struct SparseFactor{
	int topOffset;
	int bottomOffset;
	int leftOffset;
	int rightOffset;
}SparseFactor;

typedef struct Partition {
    SparseFactor sparseFactor;
    CubicFactor cubicFactor;
    struct Partition *next;
} Partition;

typedef struct Queue {
	struct Partition *head;
	struct Partition *tail;
	int size;
} Queue;

struct Image *createImage(unsigned char*, int, int, int);
struct Image *readImage(char*);
struct Image *copyImage(struct Image*);
void freeImage(struct Image*);
unsigned char min(unsigned char, unsigned char);
unsigned char max(unsigned char, unsigned char);
unsigned char getPixel(struct Image*, int, int);
void setPixel(struct Image*, int , int, unsigned char);
int getWidth(struct Image*);
int getHeight(struct Image*);
int getChannels(struct Image*);
unsigned char *getData(struct Image*);
unsigned char *dilate(unsigned char*, int, unsigned char*, unsigned char*, unsigned char*, int);
unsigned char *erode(unsigned char*, int, unsigned char*, unsigned char*, unsigned char*, int);
void dilation(struct Image*, unsigned char*, int, int);
void erosion(struct Image*, unsigned char*, int, int);
void morphOpening(struct Image*, int);
void morphClosing(struct Image*, int);
void grayscaleToBinary(struct Image*, int);
void printBinaryImage(struct Image*);
struct Image *computeBinaryDiscSE(int);
struct Image *imageUnion(struct Image*, int);
struct Image *imageIntersection(struct Image*, int);
unsigned char *dilateRow(unsigned char*, int, unsigned char*, unsigned char*, unsigned char*, int);
void rgbToGrayscale(struct Image*);
void rgbaToGrayscale(struct Image*);
Queue *newQueue();
void enqueue(struct Queue*, struct Partition);
Partition *dequeue(Queue*);
struct Image *computeCubicFactor(int, int);
void removePartition(struct Image*);
int isEmpty(struct Queue*);
#endif