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
  unsigned int size;
} Queue;

struct Image *createImage(Pixel*, int, int, int);
struct Image *readImage(char*);
struct Image *copyImage(struct Image*);
void freeImage(struct Image*);
Pixel getPixel(struct Image*, int, int);
void setPixel(struct Image*, int , int, Pixel);
int getWidth(struct Image*);
int getHeight(struct Image*);
int getChannels(struct Image*);
Pixel *getData(struct Image*);
void dilateHorizontal(Pixel*, int, Pixel*, Pixel*, int);
void dilateVertical(Pixel*, int, Pixel*, Pixel*, int, int);
void dilate3Horizontal(Pixel*, int);
void dilate3Vertical(Pixel*, int, int);
void erodeHorizontal(Pixel*, int, Pixel*, Pixel*, int);
void erodeVertical(Pixel*, int, Pixel*, Pixel*, int, int);
void erode3Horizontal(unsigned char*, int);
void erode3Vertical(unsigned char*, int, int);
void dilation(struct Image*, int, int);
void erosion(struct Image*, int, int);
void morphOpening(struct Image*, struct Partition);
void morphClosing(struct Image*, struct Partition);
void grayscaleToBinary(struct Image*, int);
void printBinaryImage(struct Image*);
struct Image *computeBinaryDiscSE(int);
void imageUnion(struct Image*, int);
void imageIntersection(struct Image*, int);
void rgbToGrayscale(struct Image*);
void rgbaToGrayscale(struct Image*);
Queue *newQueue();
void enqueue(struct Queue*, struct Partition*);
Partition *dequeue(Queue*);
struct Image *computeCubicFactor(int, int);
void removePartition(struct Image*);
int isEmpty(struct Queue*);
void dilateNaive(struct Image*, SparseFactor);
void erodeNaive(struct Image*, SparseFactor);
Partition *smallestMorphOpening(struct Image*);
void removePartition(struct Image *);
void decompose(Image*, Queue*);
#endif