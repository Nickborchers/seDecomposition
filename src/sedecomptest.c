#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "graymorph.c"		
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

// STBI_grey       = 1
// STBI_grey_alpha = 2
// STBI_rgb        = 3
// STBI_rgb_alpha  = 4

int main(int argc, char *argv[]){
	int i;
	int width = 15;
	int height = 1;
	int channels = 1;
	unsigned char test[] = {255,233, 233 ,123 ,123 ,123 ,45 ,45 ,45 ,45 ,55, 233 ,123 ,123 ,111};
	unsigned char *test1 = malloc(width*sizeof(unsigned char));

	for(i=0; i<15; i++) test1[i] = test[i];
	// for(i=0; i<width; i++) fprintf(stderr, "test1[%d]: %u\n", i, test1[i]);
	// horizontalDilation(test1, 15, 1, 1, NULL, 3);
	
	test1 = horizontalErosion(test1, 15, 1, channels, NULL, 3);

	for(i=0; i<width; i++) fprintf(stderr, "test1[%d]: %u\n", i, test1[i]);


	free(test1);
	return 0;
}
