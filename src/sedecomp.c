#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "image.c"

// STBI_grey       = 1
// STBI_grey_alpha = 2
// STBI_rgb        = 3
// STBI_rgb_alpha  = 4

#define SE_SIZE 5
#define GRAYSCALE_TO_BINARY_THRESHOLD 100

int main(int argc, char *argv[]){
	if( argc <= 1 ) {
		printf("Image name not provided in command line, program will now exit.\n");
		return 0;
	}

	char *str = argv[1];

	Image *im;
	// Some examples:

	im = readImage(str);
	if( getChannels(im) == 3) rgbToGrayscale(im);
	if( getChannels(im) == 4) rgbaToGrayscale(im);
	grayscaleToBinary(im, GRAYSCALE_TO_BINARY_THRESHOLD);
	writeImage(im, "grayToBinary.png");
	freeImage(im);


	im = computeBinaryDiscSE(5)	;
	printBinaryImage(im);
	writeImage(im, "euclideanSphere.png");
	smallestMorphClosing(im);
	free(im);
	// Image *SE, *CSE, *RSE;

	// while(RSE != NULL){

	// }

	
	return 0;
}
