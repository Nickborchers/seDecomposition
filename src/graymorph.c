/*
 * Horizontal and vertical dilation and erosion according to the algorithm presented by Van Herk/Gil-Werman
 * Implemented by Nick Borchers
*/

unsigned char min(unsigned char a, unsigned char b){
	if( a < b ) return a;
	return b;
}

unsigned char max(unsigned char a, unsigned char b){
	if( a > b ) return a;
	return b;
}

unsigned char getArray(unsigned char *a, int row, int column, int width, int height){
	return a[row*height + column];
}

void setArray(unsigned char *a, int row, int column, int width, int height, unsigned char val){
	a[row*height + column] = val;
}

unsigned char *horizontalDilation(unsigned char *a, 
						int n, 
						int height,
						int channels, 
						unsigned char *se, 
						int s){
	int u, l, i, row;
	unsigned char *b = malloc(n*height*channels*sizeof(unsigned char)); // buffer
	unsigned char *c = malloc( (s - 1) * sizeof(unsigned char)); // left max array
	unsigned char *d = malloc( (s - 1) * sizeof(unsigned char)); // right max array	
	assert(b != NULL);
	assert(c != NULL);
	assert(d != NULL);

	l = s/2;
	
	for(i = 0; i < n*height; i++) b[i] = a[i];
	
	for( row=0; row<height; row++){
		for( u = l; u < n; u += s - 1){

			d[0] = a[row*n + u];
			for(i = 1; i < s - 1; i++)
				d[i] = max(d[i - 1], a[row* n + u + 1]);
			c[s - 2] = a[row * n + u - 1];
			for( i = 1; i < s - 1; i++){
				c[s - i - 2] = max(c[s - i - 1], a[row * n + u-i-1]);
			}
			for(i = 0; i < s - 1; i++){
				
				b[row * n + u - l + i] = max(c[i], d[i]);
			}
		}
	}

	free(a);
	free(c);
	free(d);
	return b;
}

unsigned char *verticalDilation(unsigned char *a, 
						int n, 
						int height,
						int channels, 
						unsigned char *se, 
						int s){
	int u, l, i, column;
	unsigned char *b = malloc(n*height*channels*sizeof(unsigned char)); // buffer
	unsigned char *c = malloc( (s - 1) * sizeof(unsigned char)); // left max array
	unsigned char *d = malloc( (s - 1) * sizeof(unsigned char)); // right max array	
	assert(b != NULL);
	assert(c != NULL);
	assert(d != NULL);

	l = s/2;

	for(i = 0; i < n*height; i++) b[i] = a[i];
	
	for( column=0; column < n; column++){
		for( u = l; u < n; u += s - 1){

			d[0] = getArray(a, u, column, n, height);
			for(i = 1; i < s - 1; i++){
				d[i] = max(d[i - 1], getArray(a, u+1, column, n, height));
			}
			c[s - 2] = getArray(a, u - 1, column, n, height);
			for( i = 1; i < s - 1; i++){
				 c[s - i - 2] = max(c[s - i - 1], getArray(a, u - i - 1, column, n, height));
			}
			for(i = 0; i < s - 1; i++){
				setArray(b, u - l + i, column, n, height, max(c[i], d[i]));
			}
		}
	}
	free(a);
	free(c);
	free(d);
	return b;
}



unsigned char *horizontalErosion(unsigned char *a, 
						int n, 
						int height,
						int channels, 
						unsigned char *se, 
						int s){
	int u, l, i, row;
	unsigned char *b = malloc(n*height*channels*sizeof(unsigned char)); // buffer
	unsigned char *c = malloc( (s - 1) * sizeof(unsigned char)); // left max array
	unsigned char *d = malloc( (s - 1) * sizeof(unsigned char)); // right max array	
	assert(b != NULL);
	assert(c != NULL);
	assert(d != NULL);

	l = s/2;
	
	for(i = 0; i < n*height; i++) b[i] = a[i];
	
	for( row=0; row<height; row++){
		for( u = l; u < n; u += s - 1){

			d[0] = a[row*n + u];
			for(i = 1; i < s - 1; i++)
				d[i] = min(d[i - 1], a[row* n + u + 1]);
			c[s - 2] = a[row * n + u - 1];
			for( i = 1; i < s - 1; i++){
				c[s - i - 2] = min(c[s - i - 1], a[row * n + u-i-1]);
			}
			for(i = 0; i < s - 1; i++){		
				b[row * n + u - l + i] = min(c[i], d[i]);
			}
		}
	}

	free(a);
	free(c);
	free(d);
	return b;
}


unsigned char *verticalErosion(unsigned char *a,
						int n, 
						int height,
						int channels, 
						unsigned char *se, 
						int s){
	int u, l, i, column;
	unsigned char *b = malloc(n*height*channels*sizeof(unsigned char)); // buffer
	unsigned char *c = malloc( (s - 1) * sizeof(unsigned char)); // left min array
	unsigned char *d = malloc( (s - 1) * sizeof(unsigned char)); // right min array	
	assert(b != NULL);
	assert(c != NULL);
	assert(d != NULL);

	l = s/2;

	for(i = 0; i < n*height; i++) b[i] = a[i];
	
	for( column=0; column < n; column++){
		for( u = l; u < n; u += s - 1){

			d[0] = getArray(a, u, column, n, height);
			for(i = 1; i < s - 1; i++){
				d[i] = min(d[i - 1], getArray(a, u+1, column, n, height));
			}
			c[s - 2] = getArray(a, u - 1, column, n, height);
			for( i = 1; i < s - 1; i++){
				 c[s - i - 2] = min(c[s - i - 1], getArray(a, u - i - 1, column, n, height));
			}
			for(i = 0; i < s - 1; i++){
				setArray(b, u - l + i, column, n, height, min(c[i], d[i]));
			}
		}
	}
	free(a);
	free(c);
	free(d);
	return b;
}