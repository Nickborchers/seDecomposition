#ifndef IMAGE
#define IMAGE

typedef struct Image {
	int width;
	int height;
	int channels;
	int stride;
	unsigned char *data;
}Image;

typedef struct CubicFactor{
	int width;
	int height;
}CubicFactor;

typedef struct Coordinate{
	int row;
	int col;
}Coordinate;

typedef struct SparseFactor{
	Coordinate coords[4];
	int len;
}SparseFactor;

typedef struct Node {
    SparseFactor sparseFactor;
    CubicFactor cubicFactor;
    struct Node *next;
} Node;

typedef struct Queue {
	struct Node *head;
	struct Node *tail;
	int size;
} Queue;

Image *createImage(unsigned char*, int, int, int);
Image *readImage(char*);
Image *copyImage(struct Image*);
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
Image *computeBinaryDiscSE(int);
Image *imageUnion(struct Image*, int);
Image *imageIntersection(struct Image*, int);
unsigned char *dilateRow(unsigned char*, int, unsigned char*, unsigned char*, unsigned char*, int);
void rgbToGrayscale(struct Image*);
void rgbaToGrayscale(struct Image*);
Queue *newQueue();
Node *newNode(struct CubicFactor, struct SparseFactor);
void enqueue(struct Queue*, struct CubicFactor, struct SparseFactor);
Node *dequeue(Queue*);
#endif