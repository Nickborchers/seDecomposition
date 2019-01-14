/* Fast dilation and erosion with arbitrary 2-D structuring elements */
/* July 9 - November 7, 2006  Erik R. Urbach */
/* Compilation: gcc -ansi -pedantic -Wall -O3 -march=i686 -o uwmorphops UWmorphops.c -lm */

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define false 0
#define true  1

#define NUMLEVELS 65536

#define CONNECTIVITY  4
#define M_PI		3.14159265358979323846

#define MAX(a,b)  (((a)>=(b)) ? (a):(b))
#define MIN(a,b)  (((a)<=(b)) ? (a):(b))

typedef unsigned int uint;
typedef unsigned long ulong;

typedef short bool;
#define false 0
#define true  1

typedef unsigned char ubyte;
typedef uint Pixel;


typedef struct ImageGray ImageGray;

struct ImageGray
{
   long MinX;
   long MinY;
   ulong Width;
   ulong Height;
   Pixel *Pixmap;
};



typedef struct ChordListHor ChordListHor;
typedef struct ChordListVer ChordListVer;
typedef struct Chord Chord;
typedef struct MBuffer MBuffer;

struct ChordListHor
{
   long NumLengths;
   long *ChordLengths;
   long NumSE;
   long *ChordsPerSE;
   Chord **Chords;
   long MinXOffset;  /* Minimum X offset in any of the S.E. chord */
   long MaxXExtend;  /* Number of pixels union of S.E.s extend at right side
                        of image */
   long MinYOffset;
   long MaxYOffset;
};

struct ChordListVer
{
   long NumLengths;
   long *ChordLengths;
   long NumSE;
   long *ChordsPerSE;
   Chord **Chords;
   long MinXOffset;
   long MaxXOffset;
   long MaxYOffset;  /* Miximum Y offset in any of the S.E. chords */
   long MinY;
};

struct Chord
{
   long YOffset;
   long XOffset;
   long LengthIndex;
};

struct MBuffer
{
   long Width;  /* Extended width based on image width */
   long Height;  /* Height of buffer = height of union of S.E.s */
   long CurY;
   long BaseLine;  /* Which line in MBuffer.Values represent y=0 in S.E. */
   long NumLengths;  /* Number of chord lengths of all S.E.s */
   uint ***Values;  /* Array to store computed min/max values per per image
                      line, per S.E. length, and per image column */
   uint **ValuesFirstRow;
   uint *ValuesFirstRowCol;
};



void ChordListHorDelete(ChordListHor *list);
void ChordListVerDelete(ChordListVer *list);



ImageGray *ImageGrayCreate(long minx, long miny, ulong width, ulong height)
{
   ImageGray *img;

   img = malloc(sizeof(ImageGray));
   if (img==NULL)  return(NULL);
   img->MinX = minx;
   img->MinY = miny;
   img->Width = width;
   img->Height = height;
   img->Pixmap = malloc(width*height*sizeof(uint));
   if (img->Pixmap==NULL)
   {
      free(img);
      return(NULL);
   }
   return(img);
} /* ImageGrayCreate */

ImageGray *ImageGrayClone(ImageGray *in)
{
   ImageGray *img = ImageGrayCreate(in->MinX, 
				    in->MinY, 
				    in->Width, 
				    in->Height);

   int i, imagesize= in->Width * in->Height;
   if (img!=NULL){
     for (i=0; i<imagesize;i++)
       img->Pixmap[i] = in->Pixmap[i];
   }

   return(img);

} /* ImageClone */


void ImageGrayDelete(ImageGray *img)
{
   free(img->Pixmap);
   free(img);
} /* ImageGrayDelete */



void ImageGrayClear(ImageGray *img, uint v)
{
   ulong p;

   for (p=0; p<(img->Width)*(img->Height); p++)  img->Pixmap[p] = v;
} /* ImageGrayClear */



void ImageGrayReflect(ImageGray *img, ImageGray *out)
{
   ulong x, y, pc, pm;
   uint v;

   out->MinX = 1 - img->MinX - img->Width;
   out->MinY = 1 - img->MinY - img->Height;
   for (y=0; y<img->Height; y++)
   {
      for (x=0; x<img->Width; x++)
      {
         pc = y*(img->Width)+x;
         pm = (img->Height-1-y)*(img->Width)+img->Width-1-x;
         v = img->Pixmap[pc];
         out->Pixmap[pc] = img->Pixmap[pm];
         out->Pixmap[pm] = v;
      }
   }
} /* ImageGrayReflect */



void ImageGrayComplement(ImageGray *img, ImageGray *out)
{
   ulong p;

   for (p=0; p<(img->Width)*(img->Height); p++)
   {
      out->Pixmap[p] = NUMLEVELS - img->Pixmap[p] - 1;
   }
} /* ImageGrayReflect */



void ImageGrayDifference(ImageGray *a, ImageGray *b, ImageGray *out)
{
   ulong p;
   long v;

   for (p=0; p<(a->Width)*(a->Height); p++)
   {
      v = a->Pixmap[p] - b->Pixmap[p];
      if (v<0)  v = 0;
      else if (v>=NUMLEVELS)  v = NUMLEVELS-1;
      out->Pixmap[p] = v;
   }
} /* ImageGrayReflect */



ImageGray *ImageGrayPGMBinRead(char *fname)
{
   FILE *infile;
   ImageGray *img;
   ubyte *buffer;
   ulong width, height, maxval, p;
   int c, bpp=1;

   infile = fopen(fname, "rb");
   if (infile==NULL)  return(NULL);
   fscanf(infile, "P5\n");
   while ((c=fgetc(infile)) == '#')
      while ((c=fgetc(infile)) != '\n');
   ungetc(c, infile);
   fscanf(infile, "%lu %lu\n%lu\n", &width, &height, &maxval);
   img = ImageGrayCreate(0, 0, width, height);
   if (img==NULL)
   {
      fclose(infile);
      return(NULL);
   }
   if (maxval>255)  bpp = 2;
   buffer = malloc(width*height*bpp);
   if (buffer==NULL)
   {
      ImageGrayDelete(img);
      fclose(infile);
      return(NULL);
   }
   fread(buffer, bpp, width*height, infile);
   fclose(infile);
   if (bpp==1)
   {
      for (p=0; p<width*height; p++)  img->Pixmap[p] = (int)buffer[p];
   }
   else if (bpp==2)
   {
      for (p=0; p<width*height; p++)
      {
	//img->Pixmap[p] = 256*(int)buffer[p*2] + (int)buffer[p*2+1];
	img->Pixmap[p] = (int)buffer[p*2];
      }
   }
   free(buffer);
   return(img);
} /* ImageGrayPGMBinRead */



int ImageGrayPGMBinWrite(ImageGray *img, char *fname)
{
   FILE *outfile;
   ubyte *buffer;
   ulong p;

   outfile = fopen(fname, "wb");
   if (outfile==NULL)  return(-1);
   buffer = malloc((img->Width)*(img->Height));
   if (buffer==NULL)
   {
      fclose(outfile);
      return(-1);
   }
   for (p=0; p<(img->Width)*(img->Height); p++)
   {
      buffer[p] = img->Pixmap[p];
      //     buffer[p*2+1] = img->Pixmap[p] & 255;
   }
   fprintf(outfile, "P5\n%ld %ld\n%d\n", img->Width, img->Height, 255);
   fwrite(buffer, 1, (size_t)((img->Width)*(img->Height)), outfile);
   free(buffer);
   fclose(outfile);
   return(0);
} /* ImageGrayPGMBinWrite */



uint ImageGrayReadPixel(ImageGray *img, long x, long y)
{
   x -= img->MinX;
   y -= img->MinY;
   if ((x>=0) && (y>=0) && (x<(img->Width)) && (y<(img->Height)))  return(img->Pixmap[y*(img->Width)+x]);
   else  return(0);
} /* ImageGrayWritePixel */



void ImageGrayWritePixel(ImageGray *img, long x, long y, uint value)
{
   x -= img->MinX;
   y -= img->MinY;
   if ((x>=0) && (y>=0) && (x<(img->Width)) && (y<(img->Height)))  img->Pixmap[y*(img->Width)+x] = value;
} /* ImageGrayWritePixel */


void ImageGrayMinImage(ImageGray *a, 
		       ImageGray *b,
		       ImageGray *out )
{
   ulong p;
 
   for (p=0; p<(a->Width)*(a->Height); p++)
   {
       out->Pixmap[p] = (a->Pixmap[p] <= b->Pixmap[p]) ? 
	   a->Pixmap[p] : b->Pixmap[p] ;
   }
}


void ImageGrayMaxImage(ImageGray *a, 
		       ImageGray *b,
		       ImageGray *out )
{
   ulong p;
 
   for (p=0; p<(a->Width)*(a->Height); p++)
   {
       out->Pixmap[p] = (a->Pixmap[p] >= b->Pixmap[p]) ? 
	   a->Pixmap[p] : b->Pixmap[p] ;
   }
}

int ImageGraySameShape(ImageGray *a, 
			  ImageGray *b)
{
   
    return (a->Width == b->Width) && (a->Height == b->Height);
}

int ImageGrayIdentical(ImageGray *a, 
			  ImageGray *b)
{
   ulong p;
 
   if (!ImageGraySameShape(a,b))
       return 0;

   for (p=0; p<(a->Width)*(a->Height); p++)
   {
       if (a->Pixmap[p] != b->Pixmap[p]) 
	   return 0;
   }
   return 1;
}

void ImageGrayThreshold(ImageGray *img, uint value)
{
   ulong p;

   for (p=0; p<(img->Width)*(img->Height); p++)
   {
      if (img->Pixmap[p]>=value)  img->Pixmap[p] = NUMLEVELS-1;
      else  img->Pixmap[p] = 0;
   }
} /* ImageGrayThreshold */



ImageGray *ImageGrayCreateSERectangle(long minx, long miny, ulong w, ulong h)
{
   ImageGray *se;

   se = ImageGrayCreate(minx, miny, w, h);
   if (se==NULL)  return(se);
   ImageGrayClear(se, NUMLEVELS-1);
   return(se);
} /* ImageGrayCreateSERectangle */



void ImageGrayDrawVerLine(ImageGray *img, int x, int y1, int y2)
{
   int y;

   if (y2<y1)
   {
      y = y2;
      y2 = y1;
      y1 = y;
   }
   for (y=y1; y<=y2; y++)  img->Pixmap[y*(img->Width)+x] = NUMLEVELS-1;
}



ImageGray *ImageGrayCreateSEEllipse(long minx, long miny, ulong w, ulong h)
{
   ImageGray *se;
   int rx, ry, xc, yc, ry2, rx2, twory2, tworx2, x, y, p, px, py;

   se = ImageGrayCreate(minx, miny, w, h);
   if (se==NULL)  return(se);
   ImageGrayClear(se, 0);

   rx = (w-1)/2;
   ry = (h-1)/2;
   xc = w/2;
   yc = h/2;

   ry2 = ry*ry;
   rx2 = rx*rx;
   twory2 = 2*ry2;
   tworx2 = 2*rx2;
   /* region 1 */
   x = 0; y = ry;
   ImageGrayDrawVerLine(se, xc, yc-ry, yc+ry);
   p = ry2-rx2*ry+(rx2+2)/4;
   px = 0;
   py = tworx2*y;
   while (px<py)
   {
      x++;
      px += twory2;
      if (p>=0)
      {
         y--;
         py -= tworx2;
         p -= py;
      }
      p += ry2+px;
      ImageGrayDrawVerLine(se, xc-x, yc-y, yc+y);
      ImageGrayDrawVerLine(se, xc+x, yc-y, yc+y);
   }
   /* region 2 */
   p = ry2*(x+0.5)*(x+0.5) + rx2*(y-1.0)*(y-1.0) - rx2*ry2 + 0.5;
   while (y>0)
   {
      y--;
      py -= tworx2;
      if (p<=0)
      {
         x++;
         px += twory2;
         p += px;
      }
      p += rx2-py;
      ImageGrayDrawVerLine(se, xc-x, yc-y, yc+y);
      ImageGrayDrawVerLine(se, xc+x, yc-y, yc+y);
   }
   return(se);
} /* ImageGrayCreateSEEllipse */



ImageGray *ImageGrayCreateSELetterH(long minx, long miny, ulong w, ulong h)
{
   ImageGray *se;
   long i;

   se = ImageGrayCreate(minx, miny, w, h);
   if (se==NULL)  return(se);
   ImageGrayClear(se, 0);
   for (i=0; i<h; i++)  se->Pixmap[i*w] = se->Pixmap[(i+1)*w-1] = NUMLEVELS-1;
   for (i=1; i<w-1; i++)  se->Pixmap[(h/2)*w+i] = NUMLEVELS-1;
   return(se);
} /* ImageGrayCreateSELetterH */

ImageGray *ImageGrayCreateSEcross(long minx, long miny, ulong w, ulong h)
{
   ImageGray *se;
   long i;

   se = ImageGrayCreate(minx, miny, w, h);
   if (se==NULL)  return(se);
   ImageGrayClear(se, 0);
   for (i=0; i<h; i++)  se->Pixmap[i*w - minx] = NUMLEVELS-1;
   for (i=0; i<w; i++)  se->Pixmap[(h/2)*w+i] = NUMLEVELS-1;
   return(se);
}

static void CompRotMatrix (float angle, float rot_matrix[2][2])
/* Rotates foreground pixels in image in, and stores result in out.
   Rotation is by angle DEGREES, and centre of rotation is given by
   (cen_x, cen_y); */
{
   angle *= -(6.28318530718 / 360.0);
   rot_matrix[0][0] = cos((double)angle);
   rot_matrix[0][1] = sin((double)angle);
   rot_matrix[1][0] = -1.0 * sin((double)angle);
   rot_matrix[1][1] = cos((double)angle);
} /* CompRotMatrix */



static void RotatePoint(float x, float y, float cen_x, float cen_y,
                        float rot_matrix[2][2], float *new_x, float *new_y)
{
   x = x-cen_x;
   y = y-cen_y;
   *new_x = rot_matrix[0][0]*x + rot_matrix[0][1]*y + cen_x;
   *new_y = rot_matrix[1][0]*x + rot_matrix[1][1]*y + cen_y;
} /* RotatePoint */



void ImageGrayRotate(ImageGray *in, ImageGray *out, float cen_x, float cen_y, float angle)
{
   long x1, y1, x2, y2, x, y;
   float old_x, old_y, wx, wy, gleft, gright;
   long xmin, xmax, ymin, ymax;
   float rot_matrix[2][2];

   x1 = out->MinX;
   y1 = out->MinY;
   x2 = x1 + out->Width - 1;
   y2 = y1 + out->Height - 1;
   CompRotMatrix(-angle, rot_matrix);
   for (y=y1; y<=y2; y++)
   {
      for (x=x1; x<=x2; x++) 
      {
         RotatePoint((float)x, (float)y, cen_x, cen_y, rot_matrix, &old_x, &old_y);
         xmin = (int)floor( old_x );
         wx = old_x - (float) xmin;
         ymin = (int)floor( old_y );
         wy = old_y - (float) ymin;
         xmax = xmin + 1;
         ymax = ymin + 1;
         gright = (1.0-wy) * ImageGrayReadPixel(in,xmax,ymin) + wy * ImageGrayReadPixel(in,xmax,ymax);
         gleft =  (1.0-wy) * ImageGrayReadPixel(in,xmin,ymin) + wy * ImageGrayReadPixel(in,xmin,ymax);
         ImageGrayWritePixel(out, x, y, floor((1.0-wx)*gleft + wx*gright + 0.5));
      }
   }
} /* ImageGrayRotate */



void RotateCoord(float x, float y, float cx, float cy, float angle, float *rx, float *ry)
{
   angle = angle*M_PI/180.0;
   x -= cx;
   y -= cy;
   *rx = cx + x*cos(angle) - y*sin(angle);
   *ry = cy + x*sin(angle) + y*cos(angle);
} /* RotateCoord */



ImageGray *CreateRotatedImage(ImageGray *img, float angle)
{
   ImageGray *out;
   float x1, y1, x2, y2, x3, y3, x4, y4;
   long xmin, ymin, xmax, ymax;
   long rxmin, rymin, rxmax, rymax;

   xmin = img->MinX;
   ymin = img->MinY;
   xmax = xmin + img->Width - 1;
   ymax = ymin + img->Height - 1;
   RotateCoord(xmin, ymin, 0, 0, angle, &x1, &y1);
   RotateCoord(xmax, ymin, 0, 0, angle, &x2, &y2);
   RotateCoord(xmin, ymax, 0, 0, angle, &x3, &y3);
   RotateCoord(xmax, ymax, 0, 0, angle, &x4, &y4);
   rxmin = floor(MIN(x1, MIN(x2, MIN(x3, x4))) + 0.5);
   rymin = floor(MIN(y1, MIN(y2, MIN(y3, y4))) + 0.5);
   rxmax = floor(MAX(x1, MAX(x2, MAX(x3, x4))) + 0.5);
   rymax = floor(MAX(y1, MAX(y2, MAX(y3, y4))) + 0.5);
   out = ImageGrayCreate(rxmin, rymin, rxmax-rxmin+1, rymax-rymin+1);
   if (out==NULL)  return(NULL);
   ImageGrayRotate(img, out, 0, 0, angle);
   return(out);
} /* CreateRotatedImage */



ChordListHor *ChordListHorCompute(ImageGray **se, long numse)
{
   ChordListHor *chordlist;
   Chord *chord;
   long *chordsperse, *lengthtable;
   long maxsewidth, minxoffset, maxxextend, minyoffset, maxyoffset, s, y, x, len, xextend, maxlen, numlengths, sx, sy;
   uint prev, v;

   /* Find max. S.E. width */
   maxsewidth = se[0]->Width;
   for (s=1; s<numse; s++)
   {
      if (se[s]->Width > maxsewidth)  maxsewidth = se[s]->Width;
   }

   /* Find max. chord length and number of chords per S.E. */
   lengthtable = calloc(maxsewidth+1, sizeof(long));
   if (lengthtable==NULL)  return(NULL);
   chordsperse = calloc(numse, sizeof(long));
   if (chordsperse==NULL)
   {
      free(lengthtable);
      return(NULL);
   }
   minxoffset = LONG_MAX;
   maxxextend = LONG_MIN;
   minyoffset = LONG_MAX;
   maxyoffset = LONG_MIN;
   for (s=0; s<numse; s++)
   {
      for (y=0; y<se[s]->Height; y++)
      {
         prev = false;
         for (x=0; x<se[s]->Width; x++)
         {
            sx = x + se[s]->MinX;
            sy = y + se[s]->MinY;
            v = se[s]->Pixmap[y*(se[s]->Width) + x];
            if ((!prev) && (v))
            {
               prev = true;
               len = 1;
	       if (sx<minxoffset)  minxoffset = sx;
	       if (sy<minyoffset)  minyoffset = sy;
	       if (sy>maxyoffset)  maxyoffset = sy;
            }
            else if (prev)
            {
               if (v)  len++;
               else
               {
                  lengthtable[len] = true;
                  chordsperse[s]++;
                  prev = false;
                  xextend = sx;
                  if (xextend > maxxextend)  maxxextend = xextend;
               }
            }
         }
	 if (prev)
         {
            lengthtable[len] = true;
            chordsperse[s]++;
            prev = false;
            xextend = sx;
            if (xextend > maxxextend)  maxxextend = xextend;
         }
      }
   }
   lengthtable[1] = true;
   maxlen = 1;
   for (x=2; x<=maxsewidth; x++)
   {
      if (lengthtable[x])
      {
         for (y=maxlen; y<=x; y++)
	 {
            if (maxlen*2 < y)
	    {
	       lengthtable[y-1] = true;
	       maxlen = y-1;
	    }
	 }
         maxlen = x;
      }
   }

   numlengths = 1;
   for (x=2; x<=maxsewidth; x++)  if (lengthtable[x])  numlengths++;
   chordlist = malloc(sizeof(ChordListHor));
   if (chordlist==NULL)
   {
      free(chordsperse);
      free(lengthtable);
      return(NULL);
   }
   chordlist->NumLengths = numlengths;
   chordlist->ChordLengths = calloc(numlengths, sizeof(long));
   if (chordlist->ChordLengths==NULL)
   {
      ChordListHorDelete(chordlist);
      free(chordsperse);
      free(lengthtable);
      return(NULL);
   }
   y = 0;
   for (x=1; x<=maxsewidth; x++)
   {
      if (lengthtable[x])
      {
	 lengthtable[x] = y;
         chordlist->ChordLengths[y] = x;
         y++;
      }
   }
   chordlist->NumSE = numse;
   chordlist->ChordsPerSE = chordsperse;
   chordlist->Chords = calloc(numse, sizeof(Chord *));
   if (chordlist->Chords==NULL)
   {
      ChordListHorDelete(chordlist);
      free(lengthtable);
      return(NULL);
   }
   for (s=0; s<numse; s++)
   {
      chordlist->Chords[s] = calloc(chordsperse[s], sizeof(Chord));
      if (chordlist->Chords[s]==NULL)
      {
         ChordListHorDelete(chordlist);
         free(lengthtable);
         return(NULL);
      }
   }
   chordlist->MinXOffset = minxoffset;
   chordlist->MaxXExtend = maxxextend;
   chordlist->MinYOffset = minyoffset;
   chordlist->MaxYOffset = maxyoffset;
   for (s=0; s<numse; s++)
   {
      chord = &(chordlist->Chords[s][0]);
      for (y=0; y<se[s]->Height; y++)
      {
         prev = false;
         for (x=0; x<se[s]->Width; x++)
         {
            v = se[s]->Pixmap[y*(se[s]->Width) + x];
            if ((!prev) && (v))
            {
               prev = true;
               len = 1;
	       chord->YOffset = y + se[s]->MinY;
	       chord->XOffset = x + se[s]->MinX;
            }
            else if (prev)
            {
               if (v)  len++;
               else
               {
                  chord->LengthIndex = lengthtable[len];
                  chord ++;
                  prev = false;
               }
            }
         }
	 if (prev)
         {
            chord->LengthIndex = lengthtable[len];
            chord ++;
            prev = false;
         }
      }
   }
   return(chordlist);
} /* ChordListHorCompute */



ChordListVer *ChordListVerCompute(ImageGray **se, long numse)
{
   ChordListVer *chordlist;
   Chord *chord;
   long *chordsperse, *lengthtable;
   long maxseheight, maxyoffset, miny, minxoffset, maxxoffset, s, x, y, len, maxlen, numlengths, sy, sx;
   uint prev, v;

   /* Find max. S.E. height */
   maxseheight = se[0]->Height;
   for (s=1; s<numse; s++)
   {
      if (se[s]->Height > maxseheight)  maxseheight = se[s]->Height;
   }

   /* Find max. chord length and number of chords per S.E. */
   lengthtable = calloc(maxseheight+1, sizeof(long));
   if (lengthtable==NULL)  return(NULL);
   chordsperse = calloc(numse, sizeof(long));
   if (chordsperse==NULL)
   {
      free(lengthtable);
      return(NULL);
   }
   minxoffset = LONG_MAX;
   maxxoffset = LONG_MIN;
   miny = LONG_MAX;
   maxyoffset = LONG_MIN;
   for (s=0; s<numse; s++)
   {
      for (x=0; x<se[s]->Width; x++)
      {
         prev = false;
         for (y=0; y<se[s]->Height; y++)
         {
            sx = x + se[s]->MinX;
            sy = y + se[s]->MinY;
            v = se[s]->Pixmap[y*(se[s]->Width) + x];
            if ((v) && (sy < miny))  miny = sy;
            if ((!prev) && (v))
            {
               prev = true;
               len = 1;
               if (sx<minxoffset)  minxoffset = sx;
               if (sx>maxxoffset)  maxxoffset = sx;
            }
            else if (prev)
            {
               if (v)  len++;
               else
               {
                  lengthtable[len] = true;
                  chordsperse[s]++;
                  prev = false;
                  if (sy>maxyoffset)  maxyoffset = sy;
               }
            }
         }
	 if (prev)
         {
            lengthtable[len] = true;
            chordsperse[s]++;
            prev = false;
            if (sy > maxyoffset)  maxyoffset = sy;
         }
      }
   }
   lengthtable[1] = true;
   maxlen = 1;
   for (y=2; y<=maxseheight; y++)
   {
      if (lengthtable[y])
      {
         for (x=maxlen; x<=y; x++)
	 {
            if (maxlen*2 < x)
	    {
	       lengthtable[x-1] = true;
	       maxlen = x-1;
	    }
	 }
         maxlen = y;
      }
   }

   numlengths = 1;
   for (y=2; y<=maxseheight; y++)  if (lengthtable[y])  numlengths++;
   chordlist = malloc(sizeof(ChordListVer));
   if (chordlist==NULL)
   {
      free(chordsperse);
      free(lengthtable);
      return(NULL);
   }
   chordlist->NumLengths = numlengths;
   chordlist->ChordLengths = calloc(numlengths, sizeof(long));
   if (chordlist->ChordLengths==NULL)
   {
      ChordListVerDelete(chordlist);
      free(chordsperse);
      free(lengthtable);
      return(NULL);
   }
   x = 0;
   for (y=1; y<=maxseheight; y++)
   {
      if (lengthtable[y])
      {
	 lengthtable[y] = x;
         chordlist->ChordLengths[x] = y;
         x++;
      }
   }
   chordlist->NumSE = numse;
   chordlist->ChordsPerSE = chordsperse;
   chordlist->Chords = calloc(numse, sizeof(Chord *));
   if (chordlist->Chords==NULL)
   {
      ChordListVerDelete(chordlist);
      free(lengthtable);
      return(NULL);
   }
   for (s=0; s<numse; s++)
   {
      chordlist->Chords[s] = calloc(chordsperse[s], sizeof(Chord));
      if (chordlist->Chords[s]==NULL)
      {
         ChordListVerDelete(chordlist);
         free(lengthtable);
         return(NULL);
      }
   }
   chordlist->MinXOffset = minxoffset;
   chordlist->MaxXOffset = maxxoffset;
   chordlist->MaxYOffset = maxyoffset;
   chordlist->MinY = miny;
   for (s=0; s<numse; s++)
   {
      chord = &(chordlist->Chords[s][0]);
      for (x=0; x<se[s]->Width; x++)
      {
         prev = false;
         for (y=se[s]->Height-1; y>=0; y--)
         {
            v = se[s]->Pixmap[y*(se[s]->Width) + x];
            if ((!prev) && (v))
            {
               prev = true;
               len = 1;
	       chord->XOffset = x + se[s]->MinX;
	       chord->YOffset = y + se[s]->MinY;
            }
            else if (prev)
            {
               if (v)  len++;
               else
               {
                  chord->LengthIndex = lengthtable[len];
                  chord ++;
                  prev = false;
               }
            }
         }
	 if (prev)
         {
            chord->LengthIndex = lengthtable[len];
            chord ++;
            prev = false;
         }
      }
   }
   return(chordlist);
} /* ChordListVerCompute */



void ChordListHorDelete(ChordListHor *list)
{
   long s;

   if (list->Chords)
   {
      for (s=0; s<list->NumSE; s++)
      {
         if (list->Chords[s])  free(list->Chords[s]);
      }
      free(list->Chords);
   }
   if (list->ChordLengths)  free(list->ChordLengths);
   free(list);
} /* ChordListHorDelete */



void ChordListVerDelete(ChordListVer *list)
{
   long s;

   if (list->Chords)
   {
      for (s=0; s<list->NumSE; s++)
      {
         if (list->Chords[s])  free(list->Chords[s]);
      }
      free(list->Chords);
   }
   if (list->ChordLengths)  free(list->ChordLengths);
   free(list);
} /* ChordListVerDelete */



void ChordListHorPrint(ChordListHor *list)
{
   long i, j;

   printf("ChordListHor:\n");
   printf("\tNumLengths=%ld\n", list->NumLengths);
   printf("\tChordLengths=");
   for (i=0; i<list->NumLengths; i++)  printf(" %ld", list->ChordLengths[i]);
   printf("\n");
   printf("\tNumSE=%ld\n", list->NumSE);
   printf("\tChordsPerSE=");
   for (i=0; i<list->NumSE; i++)  printf(" %ld", list->ChordsPerSE[i]);
   printf("\n");
   printf("\tChords:\n");
   for (i=0; i<list->NumSE; i++)
   {
      printf("\t\tse=%ld | ", i);
      for (j=0; j<list->ChordsPerSE[i]; j++)
      {
         printf("YOffset=%ld XOffset=%ld LengthIndex=%ld  :  ", list->Chords[i][j].YOffset, list->Chords[i][j].XOffset, list->Chords[i][j].LengthIndex);
      }
      printf("\n");
   }
   printf("\tMinXOffset=%ld\n", list->MinXOffset);
   printf("\tMaxXExtend=%ld\n", list->MaxXExtend);
   printf("\tMinYOffset=%ld\n", list->MinYOffset);
   printf("\tMaxYOffset=%ld\n", list->MaxYOffset);
} /* ChordListHorPrint */



void ChordListVerPrint(ChordListVer *list)
{
   long i, j;

   printf("ChordListVer:\n");
   printf("\tNumLengths=%ld\n", list->NumLengths);
   printf("\tChordLengths=");
   for (i=0; i<list->NumLengths; i++)  printf(" %ld", list->ChordLengths[i]);
   printf("\n");
   printf("\tNumSE=%ld\n", list->NumSE);
   printf("\tChordsPerSE=");
   for (i=0; i<list->NumSE; i++)  printf(" %ld", list->ChordsPerSE[i]);
   printf("\n");
   printf("\tChords:\n");
   for (i=0; i<list->NumSE; i++)
   {
      printf("\t\tse=%ld | ", i);
      for (j=0; j<list->ChordsPerSE[i]; j++)
      {
         printf("XOffset=%ld YOffset=%ld LengthIndex=%ld  :  ", list->Chords[i][j].XOffset, list->Chords[i][j].YOffset, list->Chords[i][j].LengthIndex);
      }
      printf("\n");
   }
   printf("\tMinXOffset=%ld\n", list->MinXOffset);
   printf("\tMaxXOffset=%ld\n", list->MaxXOffset);
   printf("\tMaxYOffset=%ld\n", list->MaxYOffset);
   printf("\tMinY=%ld\n", list->MinY);
} /* ChordListVerPrint */



MBuffer *MBufferCreateHor(ChordListHor *list, ImageGray *img)
{
   MBuffer *buf;
   uint ***values;
   uint *line;
   long y, l;

   buf = malloc(sizeof(MBuffer));
   if (buf==NULL)  return(NULL);
   buf->Width = img->Width + list->MaxXExtend - list->MinXOffset;
   buf->Height = list->MaxYOffset + 1 - list->MinYOffset;
   buf->BaseLine = - list->MinYOffset;
   buf->NumLengths = list->NumLengths;
   buf->Values = values = calloc(buf->Height, sizeof(uint **));
   if (values==NULL)
   {
      free(buf);
      return(NULL);
   }
   buf->ValuesFirstRow = values[0] = calloc((buf->Height)*(buf->NumLengths), sizeof(uint *));
   if (values[0]==NULL)
   {
      free(values);
      free(buf);
      return(NULL);
   }
   buf->ValuesFirstRowCol = values[0][0] = 
      calloc((buf->Height)*(buf->NumLengths)*(buf->Width), sizeof(uint));
   if (values[0][0]==NULL)
   {
      free(values[0]);
      free(values);
      free(buf);
      return(NULL);
   }
   for (y=1; y<buf->Height; y++)  values[y] = values[y-1] + buf->NumLengths;
   line = values[0][0];
   for (y=0; y<buf->Height; y++)
   {
      for (l=0; l<buf->NumLengths; l++)
      {
         values[y][l] = line;
         line += buf->Width;
      }
   }
   return(buf);
} /* MBufferCreateHor */



MBuffer *MBufferCreateVer(ChordListVer *list, ImageGray *img)
{
   MBuffer *buf;
   uint ***values;
   uint *line;
   long y, l;

   buf = malloc(sizeof(MBuffer));
   if (buf==NULL)  return(NULL);
   buf->Width = img->Width + list->MaxXOffset - list->MinXOffset;
   buf->Height = list->MaxYOffset + 1 - list->MinY;
   buf->BaseLine = - list->MinY;
   buf->NumLengths = list->NumLengths;
   buf->Values = values = calloc(buf->Height, sizeof(uint **));
   if (values==NULL)
   {
      free(buf);
      return(NULL);
   }
   buf->ValuesFirstRow = values[0] = calloc((buf->Height)*(buf->NumLengths), sizeof(uint *));
   if (values[0]==NULL)
   {
      free(values);
      free(buf);
      return(NULL);
   }
   buf->ValuesFirstRow[0] = NULL;
   buf->ValuesFirstRowCol = values[0][0] = 
     calloc((buf->Height)*(buf->NumLengths)*(buf->Width), sizeof(uint));
   if (values[0][0]==NULL)
   {
      free(values[0]);
      free(values);
      free(buf);
      return(NULL);
   }
   for (y=1; y<buf->Height; y++)  values[y] = values[y-1] + buf->NumLengths;
   line = values[0][0];
   for (y=0; y<buf->Height; y++)
   {
      for (l=0; l<buf->NumLengths; l++)
      {
         values[y][l] = line;
         line += buf->Width;
      }
   }
   return(buf);
} /* MBufferCreateVer */



void MBufferDelete(MBuffer *buf)
{
   free(buf->ValuesFirstRowCol);
   free(buf->ValuesFirstRow);
   free(buf->Values);
   free(buf);
} /* MBufferDelete */



void MBufferInitMinHor(MBuffer *buf, ChordListHor *list, 
		       ImageGray *img, int ystart)
{
   uint ***values;
   long x, y, l;

   buf->CurY = ystart + list->MaxYOffset-1;
   if (buf->Height<=1)  return;
   values = buf->Values;
   memcpy(values[1][0] - list->MinXOffset, img->Pixmap, img->Width*sizeof(uint));
   for (x=0; x<-list->MinXOffset; x++)  values[1][0][x] = img->Pixmap[0];
   for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[1][0][x] = img->Pixmap[img->Width-1];
   for (l=1; l<buf->NumLengths; l++)
   {
      for (x=0; x<(buf->Width)-(list->ChordLengths[l-1]); x++)
      {
         values[1][l][x] = MIN(values[1][l-1][x], values[1][l-1][x+list->ChordLengths[l]-list->ChordLengths[l-1]]);
      }
      for (; x<buf->Width; x++)
      {
         values[1][l][x] = values[1][l][x-1];
      }
   }
   for (y=1; y<=buf->BaseLine; y++)  memcpy(values[y+1][0], values[1][0], (buf->Width)*(buf->NumLengths)*sizeof(uint));
   for (; y<buf->Height-1; y++)
   {
      memcpy(values[y+1][0] - list->MinXOffset, img->Pixmap, img->Width*sizeof(uint));
      for (x=0; x<-list->MinXOffset; x++)  values[y+1][0][x] = img->Pixmap[y*(img->Width)];
      for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[y+1][0][x] = img->Pixmap[img->Width-1];
      for (l=1; l<buf->NumLengths; l++)
      {
         for (x=0; x<(buf->Width)-(list->ChordLengths[l-1]); x++)
         {
            values[y+1][l][x] = MIN(values[y+1][l-1][x], values[y+1][l-1][x+list->ChordLengths[l]-list->ChordLengths[l-1]]);
         }
         for (; x<buf->Width; x++)
         {
            values[y+1][l][x] = values[y+1][l][x-1];
         }
      }
   }
} /* MBufferInitMinHor */



void MBufferInitMinVer(MBuffer *buf, ChordListVer *list, 
		       ImageGray *img, int ystart)
{
   uint ***values;
   long x, y, l;

   buf->CurY = ystart + list->MaxYOffset-1;
   if (buf->Height<=1)  return;
   values = buf->Values;
   memcpy(values[1][0] - list->MinXOffset, img->Pixmap, img->Width*sizeof(uint));
   for (x=0; x<-list->MinXOffset; x++)  values[1][0][x] = img->Pixmap[0];
   for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[1][0][x] = img->Pixmap[img->Width-1];
   for (l=1; l<buf->NumLengths; l++)  memcpy(values[1][l], values[1][0], (buf->Width)*sizeof(uint));
   for (y=1; y<=buf->BaseLine; y++)  memcpy(values[y+1][0], values[1][0], (buf->Width)*(buf->NumLengths)*sizeof(uint));
   for (; y<buf->Height-1; y++)
   {
      memcpy(values[y+1][0] - list->MinXOffset, img->Pixmap+y*(img->Width), img->Width*sizeof(uint));
      for (x=0; x<-list->MinXOffset; x++)  values[y+1][0][x] = img->Pixmap[y*(img->Width)];
      for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[y+1][0][x] = img->Pixmap[y*(img->Width)+img->Width-1];
      for (l=1; (l<buf->NumLengths) && (list->ChordLengths[l]<=y); l++)
      {
         for (x=0; x<buf->Width; x++)
         {
            values[y+1][l][x] = MIN(values[y+1][l-1][x], values[(y+1-list->ChordLengths[l]+list->ChordLengths[l-1])][l-1][x]);
         }
      }
   }
} /* MBufferInitMinVer */



void MBufferInitMaxHor(MBuffer *buf, ChordListHor *list, 
		       ImageGray *img, int ystart)
{
   uint ***values;
   long x, y, l;

   buf->CurY = ystart + list->MaxYOffset-1;
   if (buf->Height<=1)  return;
   values = buf->Values;
   memcpy(values[1][0] - list->MinXOffset, img->Pixmap, img->Width*sizeof(uint));
   for (x=0; x<-list->MinXOffset; x++)  values[1][0][x] = img->Pixmap[0];
   for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[1][0][x] = img->Pixmap[img->Width-1];
   for (l=1; l<buf->NumLengths; l++)
   {
      for (x=0; x<(buf->Width)-(list->ChordLengths[l-1]); x++)
      {
         values[1][l][x] = MAX(values[1][l-1][x], values[1][l-1][x+list->ChordLengths[l]-list->ChordLengths[l-1]]);
      }
      for (; x<buf->Width; x++)
      {
         values[1][l][x] = values[1][l][x-1];
      }
   }
   for (y=1; y<=buf->BaseLine; y++)  memcpy(values[y+1][0], values[1][0], (buf->Width)*(buf->NumLengths)*sizeof(uint));
   for (; y<buf->Height-1; y++)
   {
      memcpy(values[y+1][0] - list->MinXOffset, img->Pixmap, img->Width*sizeof(uint));
      for (x=0; x<-list->MinXOffset; x++)  values[y+1][0][x] = img->Pixmap[y*(img->Width)];
      for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[y+1][0][x] = img->Pixmap[img->Width-1];
      for (l=1; l<buf->NumLengths; l++)
      {
         for (x=0; x<(buf->Width)-(list->ChordLengths[l-1]); x++)
         {
            values[y+1][l][x] = MAX(values[y+1][l-1][x], values[y+1][l-1][x+list->ChordLengths[l]-list->ChordLengths[l-1]]);
         }
         for (; x<buf->Width; x++)
         {
            values[y+1][l][x] = values[y+1][l][x-1];
         }
      }
   }
} /* MBufferInitMaxHor */



void MBufferInitMaxVer(MBuffer *buf, ChordListVer *list, 
		       ImageGray *img, int ystart)
{
   uint ***values;
   long x, y, l;

   buf->CurY = list->MaxYOffset-1;
   if (buf->Height<=1)  return;
   values = buf->Values;
   memcpy(values[1][0] - list->MinXOffset, img->Pixmap, img->Width*sizeof(uint));
   for (x=0; x<-list->MinXOffset; x++)  values[1][0][x] = img->Pixmap[0];
   for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[1][0][x] = img->Pixmap[img->Width-1];
   for (l=1; l<buf->NumLengths; l++)  memcpy(values[1][l], values[1][0], (buf->Width)*sizeof(uint));
   for (y=1; y<=buf->BaseLine; y++)  memcpy(values[y+1][0], values[1][0], (buf->Width)*(buf->NumLengths)*sizeof(uint));
   for (; y<buf->Height-1; y++)
   {
      memcpy(values[y+1][0] - list->MinXOffset, img->Pixmap+y*(img->Width), img->Width*sizeof(uint));
      for (x=0; x<-list->MinXOffset; x++)  values[y+1][0][x] = img->Pixmap[y*(img->Width)];
      for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[y+1][0][x] = img->Pixmap[y*(img->Width)+img->Width-1];
      for (l=1; (l<buf->NumLengths) && (list->ChordLengths[l]<=y); l++)
      {
         for (x=0; x<buf->Width; x++)
         {
            values[y+1][l][x] = MAX(values[y+1][l-1][x], values[(y+1-list->ChordLengths[l]+list->ChordLengths[l-1])][l-1][x]);
         }
      }
   }
} /* MBufferInitMaxVer */



void MBufferUpdateMinHor(MBuffer *buf, ChordListHor *list, ImageGray *img)
{
   uint ***values = buf->Values;
   uint **row;
   long x, y, l, cv;

   row = values[0];
   for (y=0; y<buf->Height-1; y++)  values[y] = values[y+1];
   values[y] = row;
   buf->CurY ++;
   if (buf->CurY < img->Height)
   {
      cv = buf->Height-1;
      memcpy(values[cv][0] - list->MinXOffset, (img->Pixmap)+(buf->CurY)*img->Width, img->Width*sizeof(uint));
      for (x=0; x<-(list->MinXOffset); x++)  values[cv][0][x] = values[cv][0][-list->MinXOffset];
      for (x=0; x<list->MaxXExtend; x++)  values[cv][0][img->Width-list->MinXOffset+x] = values[cv][0][img->Width-list->MinXOffset-1];
      for (l=1; l<buf->NumLengths; l++)
      {
         for (x=0; x<(buf->Width)-(list->ChordLengths[l-1]); x++)
         {
            values[cv][l][x] = MIN(values[cv][l-1][x], values[cv][l-1][x+list->ChordLengths[l]-list->ChordLengths[l-1]]);
         }
         for (; x<buf->Width; x++)
         {
            values[cv][l][x] = values[cv][l][x-1];
         }
      }
   }
} /* MBufferUpdateMinHor */



void MBufferUpdateMinVer(MBuffer *buf, ChordListVer *list, ImageGray *img)
{
   uint ***values = buf->Values;
   uint **row;
   long x, y, l;

   row = values[0];
   for (y=0; y<buf->Height-1; y++)  values[y] = values[y+1];
   values[y] = row;
   buf->CurY ++;
   if (buf->CurY < img->Height)
   {
      memcpy(values[y][0] - list->MinXOffset, (img->Pixmap)+(buf->CurY)*img->Width, img->Width*sizeof(uint));
      for (x=0; x<-(list->MinXOffset); x++)  values[y][0][x] = values[y][0][-list->MinXOffset];
      for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[y][0][x] = img->Pixmap[y*(img->Width)+img->Width-1];

      for (l=1; (l<buf->NumLengths) && (list->ChordLengths[l]<=y+1); l++)
      {
         for (x=0; x<buf->Width; x++)
         {
            values[y][l][x] = MIN(values[y][l-1][x], values[(y-list->ChordLengths[l]+list->ChordLengths[l-1])][l-1][x]);
         }
      }
   }
} /* MBufferUpdateMinVer */



void MBufferUpdateMaxHor(MBuffer *buf, ChordListHor *list, ImageGray *img)
{
   uint ***values = buf->Values;
   uint **row;
   long x, y, l, cv;

   row = values[0];
   for (y=0; y<buf->Height-1; y++)  values[y] = values[y+1];
   values[y] = row;
   buf->CurY ++;
   if (buf->CurY < img->Height)
   {
      cv = buf->Height-1;
      memcpy(values[cv][0] - list->MinXOffset, (img->Pixmap)+(buf->CurY)*img->Width, img->Width*sizeof(uint));
      for (x=0; x<-(list->MinXOffset); x++)  values[cv][0][x] = values[cv][0][-list->MinXOffset];
      for (x=0; x<list->MaxXExtend; x++)  values[cv][0][img->Width-list->MinXOffset+x] = values[cv][0][img->Width-list->MinXOffset-1];
      for (l=1; l<buf->NumLengths; l++)
      {
         for (x=0; x<(buf->Width)-(list->ChordLengths[l-1]); x++)
         {
            values[cv][l][x] = MAX(values[cv][l-1][x], values[cv][l-1][x+list->ChordLengths[l]-list->ChordLengths[l-1]]);
         }
         for (; x<buf->Width; x++)
         {
            values[cv][l][x] = values[cv][l][x-1];
         }
      }
   }
} /* MBufferUpdateMaxHor */



void MBufferUpdateMaxVer(MBuffer *buf, ChordListVer *list, ImageGray *img)
{
   uint ***values = buf->Values;
   uint **row;
   long x, y, l;

   row = values[0];
   for (y=0; y<buf->Height-1; y++)  values[y] = values[y+1];
   values[y] = row;
   buf->CurY ++;
   if (buf->CurY < img->Height)
   {
      memcpy(values[y][0] - list->MinXOffset, (img->Pixmap)+(buf->CurY)*img->Width, img->Width*sizeof(uint));
      for (x=0; x<-(list->MinXOffset); x++)  values[y][0][x] = values[y][0][-list->MinXOffset];
      for (x=(img->Width)-(list->MinXOffset); x<buf->Width; x++)  values[y][0][x] = img->Pixmap[y*(img->Width)+img->Width-1];

      for (l=1; (l<buf->NumLengths) && (list->ChordLengths[l]<=y+1); l++)
      {
         for (x=0; x<buf->Width; x++)
         {
            values[y][l][x] = MAX(values[y][l-1][x], values[(y-list->ChordLengths[l]+list->ChordLengths[l-1])][l-1][x]);
         }
      }
   }
} /* MBufferUpdateMaxVer */



void MBufferLineErodeHor(MBuffer *buf, ChordListHor *list, ImageGray *img, ImageGray **out)
{
   Chord *chord;
   uint *v, *outpm, *outline;
   long s, c, x;

   for (s=0; s<list->NumSE; s++)
   {
      chord = list->Chords[s];
      v = buf->Values[buf->BaseLine+chord->YOffset][chord->LengthIndex] + (chord->XOffset-list->MinXOffset);
      outpm = outline = (out[s]->Pixmap) + ((buf->CurY)-(list->MaxYOffset))*(img->Width);
      for (x=0; x<img->Width; x++)
      {
         *outpm = *v;
         v++;
         outpm++;
      }
      chord++;
      for (c=1; c<list->ChordsPerSE[s]; c++)
      {
         v = buf->Values[buf->BaseLine+chord->YOffset][chord->LengthIndex] + (chord->XOffset-list->MinXOffset);
         outpm = outline;
         for (x=0; x<img->Width; x++)
         {
            if (*v<*outpm)  *outpm = *v;
            v++;
            outpm++;
         }
         chord++;
      }
   }
} /* MBufferLineErodeHor */



void MBufferLineErodeVer(MBuffer *buf, ChordListVer *list, ImageGray *img, ImageGray **out)
{
   Chord *chord;
   uint *v, *outpm, *outline;
   long s, c, x;

   for (s=0; s<list->NumSE; s++)
   {
      chord = list->Chords[s];
      v = buf->Values[buf->BaseLine+chord->YOffset][chord->LengthIndex] + (chord->XOffset-list->MinXOffset);
      outpm = outline = (out[s]->Pixmap) + ((buf->CurY)-(list->MaxYOffset))*(img->Width);
      for (x=0; x<img->Width; x++)
      {
         *outpm = *v;
         v++;
         outpm++;
      }
      chord++;
      for (c=1; c<list->ChordsPerSE[s]; c++)
      {
         v = buf->Values[buf->BaseLine+chord->YOffset][chord->LengthIndex] + (chord->XOffset-list->MinXOffset);
         outpm = outline;
         for (x=0; x<img->Width; x++)
         {
            if (*v<*outpm)  *outpm = *v;
            v++;
            outpm++;
         }
         chord++;
      }
   }
} /* MBufferLineErodeVer */



void MBufferLineDilateHor(MBuffer *buf, ChordListHor *list, ImageGray *img, ImageGray **out)
{
   Chord *chord;
   uint *v, *outpm, *outline;
   long s, c, x;

   for (s=0; s<list->NumSE; s++)
   {
      chord = list->Chords[s];
      v = buf->Values[buf->BaseLine+chord->YOffset][chord->LengthIndex] + (chord->XOffset-list->MinXOffset);
      outpm = outline = (out[s]->Pixmap) + ((buf->CurY)-(list->MaxYOffset))*(img->Width);
      for (x=0; x<img->Width; x++)
      {
         *outpm = *v;
         v++;
         outpm++;
      }
      chord++;
      for (c=1; c<list->ChordsPerSE[s]; c++)
      {
         v = buf->Values[buf->BaseLine+chord->YOffset][chord->LengthIndex] + (chord->XOffset-list->MinXOffset);
         outpm = outline;
         for (x=0; x<img->Width; x++)
         {
            if (*v>*outpm)  *outpm = *v;
            v++;
            outpm++;
         }
         chord++;
      }
   }
} /* MBufferLineDilateHor */



void MBufferLineDilateVer(MBuffer *buf, ChordListVer *list, ImageGray *img, ImageGray **out)
{
   Chord *chord;
   uint *v, *outpm, *outline;
   long s, c, x;

   for (s=0; s<list->NumSE; s++)
   {
      chord = list->Chords[s];
      v = buf->Values[buf->BaseLine+chord->YOffset][chord->LengthIndex] + (chord->XOffset-list->MinXOffset);
      outpm = outline = (out[s]->Pixmap) + ((buf->CurY)-(list->MaxYOffset))*(img->Width);
      for (x=0; x<img->Width; x++)
      {
         *outpm = *v;
         v++;
         outpm++;
      }
      chord++;
      for (c=1; c<list->ChordsPerSE[s]; c++)
      {
         v = buf->Values[buf->BaseLine+chord->YOffset][chord->LengthIndex] + (chord->XOffset-list->MinXOffset);
         outpm = outline;
         for (x=0; x<img->Width; x++)
         {
            if (*v>*outpm)  *outpm = *v;
            v++;
            outpm++;
         }
         chord++;
      }
   }
} /* MBufferLineDilateVer */



int MultiErosion(ImageGray *img, long numse, ImageGray **se, ImageGray **out)
{
   ChordListHor *listh;
   ChordListVer *listv;
   MBuffer *buf;
   long numchordshor=0, numchordsver=0, s;

   listh = ChordListHorCompute(se, numse);
   if (listh==NULL)  return(-1);
   listv = ChordListVerCompute(se, numse);
   if (listv==NULL)  return(-1);
   for (s=0; s<listh->NumSE; s++)  numchordshor += listh->ChordsPerSE[s];
   for (s=0; s<listv->NumSE; s++)  numchordsver += listv->ChordsPerSE[s];
   //   fprintf(stderr, "numchordshor=%ld numchordsver=%ld\n", numchordshor, numchordsver);
   if (numchordshor<=numchordsver)
   {
      ChordListVerDelete(listv);
      buf = MBufferCreateHor(listh, img);
      if (buf==NULL)
      {
         ChordListHorDelete(listh);
         return(-1);
      }
      MBufferInitMinHor(buf, listh, img, 0);
      do
      {
         MBufferUpdateMinHor(buf, listh, img);
         MBufferLineErodeHor(buf, listh, img, out);
      } while (((buf->CurY)-(listh->MaxYOffset)) < img->Height-1);
      MBufferDelete(buf);
      ChordListHorDelete(listh);
   } else {
      fprintf(stderr, "MultiErosion: Ver\n");
      ChordListHorDelete(listh);
      buf = MBufferCreateVer(listv, img);
      if (buf==NULL)
      {
         ChordListVerDelete(listv);
         return(-1);
      }
      MBufferInitMinVer(buf, listv, img, 0);
      do
      {
         MBufferUpdateMinVer(buf, listv, img);
         MBufferLineErodeVer(buf, listv, img, out);
      } while (((buf->CurY)-(listv->MaxYOffset)) < img->Height-1);
      MBufferDelete(buf);
      ChordListVerDelete(listv);
   }
   return(0);
} /* MultiErosion */


void ImageGrayErosion(ImageGray *img, 
			 ImageGray *se, 
			 ImageGray *out)
{
    MultiErosion(img,1, &se, &out);
}

void ImageGrayConditionalErosion(ImageGray *img, 
                                    ImageGray *condition,
				    ImageGray *se, 
				    ImageGray *out)
{
    MultiErosion(img,1, &se, &out);
    ImageGrayMaxImage(out,condition,out);
}


int MultiDilation(ImageGray *img, long numse, ImageGray **se, ImageGray **out)
{
   ChordListHor *listh;
   ChordListVer *listv;
   MBuffer *buf;
   long numchordshor=0, numchordsver=0, s;

   listh = ChordListHorCompute(se, numse);
   if (listh==NULL)  return(-1);
   listv = ChordListVerCompute(se, numse);
   if (listv==NULL)  return(-1);
   for (s=0; s<listh->NumSE; s++)  numchordshor += listh->ChordsPerSE[s];
   for (s=0; s<listv->NumSE; s++)  numchordsver += listv->ChordsPerSE[s];
   //fprintf(stderr, "numchordshor=%ld numchordsver=%ld\n", numchordshor, numchordsver);
   if (numchordshor<=numchordsver)
   {
      ChordListVerDelete(listv);
      buf = MBufferCreateHor(listh, img);
      if (buf==NULL)
      {
         ChordListHorDelete(listh);
         return(-1);
      }
      MBufferInitMaxHor(buf, listh, img, 0);
      do
      {
         MBufferUpdateMaxHor(buf, listh, img);
         MBufferLineDilateHor(buf, listh, img, out);
      } while (((buf->CurY)-(listh->MaxYOffset)) < img->Height-1);
      MBufferDelete(buf);
      ChordListHorDelete(listh);
   } else {
      ChordListHorDelete(listh);
      buf = MBufferCreateVer(listv, img);
      if (buf==NULL)
      {
         ChordListVerDelete(listv);
         return(-1);
      }
      MBufferInitMaxVer(buf, listv, img, 0);
      do
      {
         MBufferUpdateMaxVer(buf, listv, img);
         MBufferLineDilateVer(buf, listv, img, out);
      } while (((buf->CurY)-(listv->MaxYOffset)) < img->Height-1);
      MBufferDelete(buf);
      ChordListVerDelete(listv);
   }
   return(0);
} /* MultiDilation */

void ImageGrayDilation(ImageGray *img, 
			  ImageGray *se, 
			  ImageGray *out)
{
    MultiDilation(img,1, &se, &out);
}

void ImageGrayOpening(ImageGray *img, 
		      ImageGray *se, 
		      ImageGray *out)
{
    MultiErosion(img,1, &se, &out);
    ImageGrayReflect(se,se);
    MultiDilation(out,1, &se, &out);
    ImageGrayReflect(se,se);
}

void ImageGrayClosing(ImageGray *img, 
			 ImageGray *se, 
			 ImageGray *out)
{
    MultiDilation(img,1, &se, &out);
    ImageGrayReflect(se,se);
    MultiErosion(out,1, &se, &out);
    ImageGrayReflect(se,se);
}

void ImageGrayConditionalDilation(ImageGray *img, 
                                     ImageGray *condition,
				     ImageGray *se, 
				     ImageGray *out)
{
    MultiDilation(img,1, &se, &out);
    ImageGrayMinImage(out,condition,out);
}


int MultiOpening(ImageGray *img, long numse, ImageGray **se, ImageGray **out)
{
   long s;
   int r;

   r = MultiErosion(img, numse, se, out);
   if (r)  return(r);
   for (s=0; (s<numse) && (r==0); s++)  r = MultiDilation(out[s], 1, se+s, out+s);
   return(r);
} /* MultiOpening */



int MultiClosing(ImageGray *img, long numse, ImageGray **se, ImageGray **out)
{
   long s;
   int r;

   r = MultiDilation(img, numse, se, out);
   if (r)  return(r);
   for (s=0; (s<numse) && (r==0); s++)  r = MultiErosion(out[s], 1, se+s, out+s);
   return(r);
} /* MultiClosing */



int MultiHitOrMiss(ImageGray *img, long numse, ImageGray **se1, ImageGray **se2, ImageGray **out)
{
   ImageGray **t;
   long s;
   int r;

   t = calloc(numse, sizeof(ImageGray *));
   for (s=0; s<numse; s++) t[s] = ImageGrayCreate(img->MinX, img->MinY, img->Width, img->Height);

   r = MultiErosion(img, numse, se1, t);
   if (r)  return(r);
   for (s=0; s<numse; s++)  ImageGrayReflect(se2[s], se2[s]);
   r = MultiDilation(img, numse, se2, out);
   if (r)  return(r);
   for (s=0; s<numse; s++)  ImageGrayReflect(se2[s], se2[s]);
   for (s=0; s<numse; s++)  ImageGrayDifference(t[s], out[s], out[s]);
   for (s=0; s<numse; s++)  ImageGrayDelete(t[s]);
   free(t);
   return(r);
} /* MultiHitOrMiss */


void ReconstructWithCriteria(ImageGray *orig, 
			     ImageGray *marker,
			     ImageGray *se1, 
                             ImageGray *se2,
                             ImageGray *out    )
{
    do {
	ImageGrayOpening(marker,se1,out);
	ImageGrayConditionalDilation(out,orig,se2,out);
	ImageGrayOpening(out,se1,marker);
	ImageGrayConditionalDilation(marker,orig,se2,marker);
        
    } while(!ImageGrayIdentical(marker,out));
  
}

void ReconstructWithCriteriaMovie(ImageGray *orig, 
				  ImageGray *marker,
				  ImageGray *se1, 
				  ImageGray *se2,
				  ImageGray *out,
				  char      *basefname,
                                  int width            )
{   int i=0;
 char outfname[256];
 do {
   sprintf(outfname,"%s%0*d.pgm",basefname,width,i);
   ImageGrayPGMBinWrite(marker,outfname);
   i++;
   ImageGrayOpening(marker,se1,out);
   ImageGrayConditionalDilation(out,orig,se2,out);
   ImageGrayOpening(out,se1,marker);
   ImageGrayConditionalDilation(marker,orig,se2,marker);
 } while(!ImageGrayIdentical(marker,out));
   sprintf(outfname,"%s%0*d.pgm",basefname,width,i);
   ImageGrayPGMBinWrite(marker,outfname);
  
}

void ReconstructMovie(ImageGray *orig, 
		      ImageGray *marker,
		      ImageGray *se1, 
		      ImageGray *se2,
		      ImageGray *out,
		      char      *basefname, 
		      int        width)
{   int i=0;
 char outfname[256];
 do {
   sprintf(outfname,"%s%0*d.pgm",basefname,width,i);
   ImageGrayPGMBinWrite(marker,outfname);
   i++;
   ImageGrayConditionalDilation(marker,orig,se2,out);
   ImageGrayConditionalDilation(out,orig,se2,marker);
 } while(!ImageGrayIdentical(marker,out));
   sprintf(outfname,"%s%0*d.pgm",basefname,width,i);
   ImageGrayPGMBinWrite(marker,outfname);

  
}



typedef struct HQueue
{
   ulong *Pixels;
   ulong Head;
   ulong Tail; /* First free place in queue, or -1 when the queue is full */
} HQueue;



typedef struct MaxNode MaxNode;

struct MaxNode
{
   MaxNode *Parent;
   Pixel Level;
   Pixel NewLevel;  /* gray level after filtering */
};



/* Status stores the information of the pixel status: the pixel can be
 * NotAnalyzed, InTheQueue or assigned to node k at level h. In this
 * last case Status(p)=k. */
#define ST_NotAnalyzed  -1
#define ST_InTheQueue   -2

typedef struct MaxTree MaxTree;

struct MaxTree
{
   long *Status;
   ulong *NumPixelsBelowLevel;
   ulong *NumNodesAtLevel; /* Number of nodes C^k_h at level h */
   MaxNode **Nodes;
};

int h_min;

void MaxTreeDelete(MaxTree *mt);





void ImageGrayInit(ImageGray *img, ubyte h)
{
   memset(img->Pixmap, h, (img->Width)*(img->Height));
} /* ImageGrayInit */


void ImageGrayInvert(ImageGray *img)
{
 int i, imgsize=(img->Width)*(img->Height);
 Pixel *cur = img->Pixmap;
 
 for (i=0;i<imgsize;i++,cur++)
   *cur = NUMLEVELS -1 -*cur;
}



HQueue *HQueueCreate(ulong imgsize, ulong *numpixelsperlevel)
{
   HQueue *hq;
   int i;

   hq = calloc(NUMLEVELS, sizeof(HQueue));
   if (hq==NULL)  return(NULL);
   hq->Pixels = calloc(imgsize, sizeof(ulong));
   if (hq->Pixels==NULL)
   {
      free(hq);
      return(NULL);
   }
   hq->Head = hq->Tail = 0;
   for (i=1; i<NUMLEVELS; i++)
   {
      hq[i].Pixels = hq[i-1].Pixels + numpixelsperlevel[i-1];
      hq[i].Head = hq[i].Tail = 0;
   }
   return(hq);
} /* HQueueCreate */



void HQueueDelete(HQueue *hq)
{
   free(hq->Pixels);
   free(hq);
} /* HQueueDelete */



#define HQueueFirst(hq,h)  (hq[h].Pixels[hq[h].Head++])
#define HQueueAdd(hq,h,p)  hq[h].Pixels[hq[h].Tail++] = p
#define HQueueNotEmpty(hq,h)  (hq[h].Head != hq[h].Tail)



int GetNeighbors(ulong imgwidth, ulong imgsize, ulong p,
                 ulong *neighbors)
{
   ulong x;
   int n=0;

   x = p % imgwidth;
   if (x<(imgwidth-1))   
     neighbors[n++] = p+1;
   if (p>=imgwidth){  
     neighbors[n++] = p-imgwidth;
     if (CONNECTIVITY == 8) {
       if (x<(imgwidth-1))   
	 neighbors[n++] = p-imgwidth+1;
       if (x>0)                 
	 neighbors[n++] = p-imgwidth-1;
     }

   }
   if (x>0)                 
     neighbors[n++] = p-1;
   p += imgwidth;
   if (p<imgsize){             
     neighbors[n++] = p;
     if (CONNECTIVITY == 8) {
       if (x<(imgwidth-1))   
	 neighbors[n++] = p+1;
       if (x>0)                 
	 neighbors[n++] = p-1;
     }

   }

   return(n);
} /* GetNeighbors */

void ArrayCopy(MaxNode *input, MaxNode *output, int size)
{
  int i;
  for (i=0;i<size;i++)
    {
      output[i] = input[i];
    }
} /* ArrayCopy */

void CopyIndices(ulong *input, ulong *output, int size)
{
  int i;
  for (i=0; i<size; i++)
    {
      output[i] = input[i];
    }

} /* CopyIndices */



int MaxTreeFlood(MaxTree *mt, HQueue *hq, ulong *numpixelsperlevel,
                 bool *nodeatlevel, ImageGray *img, ImageGray *marker, 
		 int h, ulong *thismaximum)
/* Returns value >=NUMLEVELS if error */
{
   ulong neighbors[CONNECTIVITY];
   Pixel *pixmap, *markermap; 
   ulong imgwidth, imgsize, p, q, idx;
   ulong maximum = *thismaximum>h ? h : *thismaximum, childmaximum;
   MaxNode *node,*parNode;
   int numneighbors, i;
   int m;

   imgwidth = img->Width;
   imgsize = imgwidth * (img->Height);
   pixmap = img->Pixmap;
   markermap = marker->Pixmap;
   while(HQueueNotEmpty(hq, h))
   {
      p = HQueueFirst(hq, h);
      if (maximum < markermap[p])
	  maximum = markermap[p] > h ? h : markermap[p];
 
      mt->Status[p] = mt->NumNodesAtLevel[h];
      numneighbors = GetNeighbors(imgwidth, imgsize, p, neighbors);
      for (i=0; i<numneighbors; i++)
      {
         q = neighbors[i];
         if (mt->Status[q]==ST_NotAnalyzed)
         {
            HQueueAdd(hq, pixmap[q], q);
            mt->Status[q] = ST_InTheQueue;
            nodeatlevel[pixmap[q]] = true;
            if (pixmap[q] > h)
            {
               m = pixmap[q];
               childmaximum = 0;
               do
               {  
                   m = MaxTreeFlood( mt, hq, numpixelsperlevel,
				     nodeatlevel,img,marker,m, &childmaximum);
                  if (m>=NUMLEVELS)
                     return(m);
               } while (m!=h);
               if (maximum < childmaximum)
		   maximum = childmaximum > h ? h : childmaximum;
            }
         }
      }
   }
   node = (mt->Nodes[h]) + mt->NumNodesAtLevel[h];
   mt->NumNodesAtLevel[h] = mt->NumNodesAtLevel[h]+1;
   m = h-1;
   while ((m>=0) && (nodeatlevel[m]==false))  m--;
   if (m>=0)
   {
     node->Parent = (mt->Nodes[m]) + mt->NumNodesAtLevel[m];
   } else {
     node->Parent = node;
   }
   //fprintf(stderr,"Assigning attributes\n");
   node->NewLevel = maximum;
   //fprintf(stderr,"maximum = %ld\n", maximum);
   //fprintf(stderr,"Attributes\n");
   node->Level = h;
   //fprintf(stderr,"Level\n");
   nodeatlevel[h] = false;
   //fprintf(stderr,"nodeatlevel\n");
   *thismaximum = maximum;
   //fprintf(stderr,"*thisarea\n");
   return(m);
} /* MaxTreeFlood */



MaxTree *MaxTreeCreate(ImageGray *img, ImageGray *marker)
{
   ulong numpixelsperlevel[NUMLEVELS];
   bool nodeatlevel[NUMLEVELS];
   HQueue *hq;
   MaxTree *mt;
   Pixel *pixmap = img->Pixmap;
   ulong imgsize, p, m=0, maximum=0;
   int l,i;

   /* Allocate structures */
   mt = malloc(sizeof(MaxTree));
   if (mt==NULL)  return(NULL);
   imgsize = (img->Width)*(img->Height);
   mt->Status = calloc((size_t)imgsize, sizeof(long));
   if (mt->Status==NULL)
   {
      free(mt);
      return(NULL);
   }
   mt->NumPixelsBelowLevel = calloc(NUMLEVELS, sizeof(ulong));
   if (mt->NumPixelsBelowLevel==NULL)
   {
      free(mt->Status);
      free(mt);
      return(NULL);
   }
   mt->NumNodesAtLevel = calloc(NUMLEVELS, sizeof(ulong));
   if (mt->NumNodesAtLevel==NULL)
   {
      free(mt->NumPixelsBelowLevel);
      free(mt->Status);
      free(mt);
      return(NULL);
   }
   mt->Nodes = calloc((size_t)NUMLEVELS, sizeof(MaxNode*));
   if (mt->Nodes==NULL){
      free(mt->NumNodesAtLevel);
      free(mt->NumPixelsBelowLevel);
      free(mt->Status);
      free(mt);
      return(NULL);
   }

   /* Initialize structures */
   for (p=0; p<imgsize; p++)  
     mt->Status[p] = ST_NotAnalyzed;
 
   bzero(nodeatlevel, NUMLEVELS*sizeof(bool));
   bzero(numpixelsperlevel, NUMLEVELS*sizeof(ulong));
   /* Following bzero is redundant, array is initialized by calloc */
   /* bzero(mt->NumNodesAtLevel, NUMLEVELS*sizeof(ulong)); */
   for (p=0; p<imgsize; p++)  
     numpixelsperlevel[pixmap[p]]++;

   for (i=0;i<NUMLEVELS;i++){
     mt->Nodes[i] = calloc(numpixelsperlevel[i],sizeof(MaxNode));
   }

   mt->NumPixelsBelowLevel[0] = 0;
   for (l=1; l<NUMLEVELS; l++)
   {
      mt->NumPixelsBelowLevel[l] = mt->NumPixelsBelowLevel[l-1] + numpixelsperlevel[l-1];
   }
   hq = HQueueCreate(imgsize, numpixelsperlevel);
   if (hq==NULL)
   {
      free(mt->Nodes);
      free(mt->NumNodesAtLevel);
      free(mt->NumPixelsBelowLevel);
      free(mt->Status);
      free(mt);
      return(NULL);
   }

   /* Find pixel m which has the lowest intensity l in the image */
   for (p=0; p<imgsize; p++)
   {
      if (pixmap[p]<pixmap[m])  m = p;
   }
   l = pixmap[m];
   h_min = l;

   /* Add pixel m to the queue */
   nodeatlevel[l] = true;
   HQueueAdd(hq, l, m);
   mt->Status[m] = ST_InTheQueue;

   /* Build the Max-tree using a flood-fill algorithm */
 
   l = MaxTreeFlood(mt, hq, numpixelsperlevel, nodeatlevel, img,
                    marker, l, &maximum);

   if (l>=NUMLEVELS)  MaxTreeDelete(mt);
   HQueueDelete(hq);
   return(mt);
} /* MaxTreeCreate */



void MaxTreeDelete(MaxTree *mt)
{
   void *attr;
   ulong i;
   int h;

   free(mt->Nodes);
   free(mt->NumNodesAtLevel);
   free(mt->NumPixelsBelowLevel);
   free(mt->Status);
   free(mt);
} /* MaxTreeDelete */


int MaxTreeFilterReconstr(MaxTree *mt, 
			  ImageGray *img,
			  ImageGray *out)
{
   MaxNode *node, *parent;
   ulong i, idx;
   int l;



   for (l=0; l<NUMLEVELS; l++)
   {
     for (i=0; i<mt->NumNodesAtLevel[l]; i++)
     {
        
       node = mt->Nodes[l] + i;
       parent = node->Parent;
       if (parent->Level<parent->NewLevel){
	 fprintf(stderr,"Houston we have a problem\nLevel= %d, NewLevel= %d\n",
		 parent->Level,parent->NewLevel);
	 exit(-1);
       }
        
       if (node->NewLevel < parent->NewLevel )
	 node->NewLevel = parent->NewLevel;
     }
   }

   for (i=0; i<(img->Width)*(img->Height); i++){
     out->Pixmap[i] = mt->Nodes[img->Pixmap[i]][mt->Status[i]].NewLevel;
   }
   return(0);
} 

void Reconstruct(ImageGray *img, ImageGray *marker, ImageGray *out){
   MaxTree *mt;
   mt = MaxTreeCreate(img, marker);
   //fprintf(stderr,"Maxtree created\n");
   MaxTreeFilterReconstr(mt, img, out);
   //fprintf(stderr,"Maxtree filtered\n");
   MaxTreeDelete(mt);

}


void Leveling(ImageGray *img,ImageGray *marker, ImageGray *out){
  Reconstruct(img,marker,out);
  ImageGrayInvert(out);
  ImageGrayInvert(marker);
  Reconstruct(out,marker,out);
  ImageGrayInvert(out);
  ImageGrayInvert(marker);  
}


void ReconstructWithCriteria2(ImageGray *img, 
			      ImageGray *marker,
			      ImageGray *se,
                              ImageGray *ball, 
                              ImageGray *out    )
{
   ImageGrayErosion(img,se,out);
   ImageGrayErosion(marker,se,marker);
   Reconstruct(out,marker,marker);
   ImageGrayReflect(se,se);
   ImageGrayDilation(marker,se,out);
   //ImageGrayConditionalDilation(out,img,ball,out);
}

void LevelingWithCriteria(ImageGray *img, 
			  ImageGray *marker,
			  ImageGray *se,
			  ImageGray *ball, 
			  ImageGray *out ){

  ImageGray *upmarker = ImageGrayClone(marker), 
    *downmarker = ImageGrayClone(marker), 
    *temp = ImageGrayClone(img);
  int i, imagesize= img->Height * img->Width;


  ReconstructWithCriteria2(img,upmarker,se,ball,temp);
  for (i= 0; i<imagesize;i++)
    temp->Pixmap[i] = (temp->Pixmap[i]>marker->Pixmap[i])? temp->Pixmap[i]:marker->Pixmap[i];

  ImageGrayInvert(img);
  ImageGrayInvert(downmarker);

  ReconstructWithCriteria2(img,downmarker,se,ball,out);

  ImageGrayInvert(img);
  ImageGrayInvert(out);
   
  for (i= 0; i<imagesize;i++)
    out->Pixmap[i] = (out->Pixmap[i]<marker->Pixmap[i])? out->Pixmap[i]:temp->Pixmap[i];
  
  ImageGrayDelete(upmarker);
  ImageGrayDelete(downmarker);
  ImageGrayDelete(temp);


      
}

int main2(int argc,char *argv[])
{
   ImageGray *img, *out, *marker, *temp, *se, *ball;
   char *imgfname, *outfname, *markerfname,*basefname;
   ulong kw, kh,r;
   int width=4;

   imgfname = argv[1];
   markerfname = argv[2];
   kw = atoi(argv[3]);
   outfname = argv[4];
   basefname = argv[5];
   if (argc > 6)
     width=atoi(argv[6]);
   

   img = ImageGrayPGMBinRead(imgfname);
   if (img==NULL)
   {
      fprintf(stderr, "Can't read image '%s'\n", imgfname);
      return(-1);
   }
   marker = ImageGrayPGMBinRead(markerfname);
   if (marker==NULL ){
      fprintf(stderr, "Can't read image '%s'\n", markerfname);
      ImageGrayDelete(img);
      return(-1);
   } else if (!ImageGraySameShape(marker,img)){
      fprintf(stderr, "Images not same shape\n");
      ImageGrayDelete(img);
      ImageGrayDelete(marker);
      return(-1);
   }   
   out = ImageGrayCreate(img->MinX, img->MinY, img->Width, img->Height);
   if (out==NULL)
   {
      fprintf(stderr, "Can't create output image\n");
      ImageGrayDelete(img);
      ImageGrayDelete(marker);
      return(-1);
   }
   temp = ImageGrayCreate(img->MinX, img->MinY, img->Width, img->Height);

   se = ImageGrayCreateSEEllipse(-(kw/2), -(kw/2), kw, kw);
   ball = ImageGrayCreateSEcross(-1, -1, 3, 3);

 
   LevelingWithCriteria(img,marker,se,ball,out);
   //ReconstructWithCriteriaMovie(img,marker,se,ball,out,basefname,width);
    
   
 
   r = ImageGrayPGMBinWrite(se, "se.pgm");
   r = ImageGrayPGMBinWrite(out, outfname);
   if (r)  fprintf(stderr, "Error (%d) writing image '%s'\n", r, outfname);
   ImageGrayDelete(se);
   ImageGrayDelete(ball);
   ImageGrayDelete(out);
   ImageGrayDelete(img);
   ImageGrayDelete(marker);
   return(0);


}























int main(int argc, char *argv[])
{
   ImageGray *img, *out, *se;
   char *imgfname, *outfname, *seshapename, *opname;
   ulong kw, kh;
   float angle;
   int r;

   if (argc!=8)
   {
      fprintf(stderr, "Usage: %s <input image> <op> <shape> <kw> <kh> <angle> <output image>\n", argv[0]);
      fprintf(stderr, "Where: op = d : dilation\n");
      fprintf(stderr, "            e : erosion\n");
      fprintf(stderr, "            o : opening\n");
      fprintf(stderr, "            c : closing\n");
      fprintf(stderr, "            h : Hit-or-Miss transform\n");
      fprintf(stderr, "       shape = r : rectangle\n");
      fprintf(stderr, "               e : ellipse\n");
      fprintf(stderr, "               h : letter H\n");
      return(0);
   }
   imgfname = argv[1];
   opname = argv[2];
   seshapename = argv[3];
   kw = atol(argv[4]);
   kh = atol(argv[5]);
   angle = atof(argv[6]);
   outfname = argv[7];

   img = ImageGrayPGMBinRead(imgfname);
   if (img==NULL)
   {
      fprintf(stderr, "Can't read image '%s'\n", imgfname);
      return(-1);
   }
   out = ImageGrayCreate(img->MinX, img->MinY, img->Width, img->Height);
   if (out==NULL)
   {
      fprintf(stderr, "Can't create output image\n");
      ImageGrayDelete(img);
      return(-1);
   }
   if (strcmp(seshapename,"r")==0)  se = ImageGrayCreateSERectangle(-(kw/2), -(kh/2), kw, kh);
   else if (strcmp(seshapename,"e")==0)  se = ImageGrayCreateSEEllipse(-(kw/2), -(kh/2), kw, kh);
   else if (strcmp(seshapename,"h")==0)  se = ImageGrayCreateSELetterH(-(kw/2), -(kh/2), kw, kh);
   else
   {
      fprintf(stderr, "Unknown shape '%s'\n", seshapename);
      ImageGrayDelete(out);
      ImageGrayDelete(img);
      return(-1);
   }
   if (se==NULL)
   {
      fprintf(stderr, "Can't create S.E.\n");
      ImageGrayDelete(out);
      ImageGrayDelete(img);
      return(-1);
   }
   se = CreateRotatedImage(se, angle);
   if (se==NULL)
   {
      fprintf(stderr, "SE rotate error!\n");
      ImageGrayDelete(out);
      ImageGrayDelete(img);
      return(-1);
   }
   ImageGrayThreshold(se, NUMLEVELS/2);
   if (strcmp(opname, "d")==0)  r = MultiDilation(img, 1, &se, &out);
   else if (strcmp(opname, "e")==0)  r = MultiErosion(img, 1, &se, &out);
   else if (strcmp(opname, "o")==0)  r = MultiOpening(img, 1, &se, &out);
   else if (strcmp(opname, "c")==0)  r = MultiClosing(img, 1, &se, &out);
   else if (strcmp(opname, "h")==0)
   {
      ImageGray *se2 = ImageGrayCreate(se->MinX, se->MinY, se->Width, se->Height);
      ImageGrayComplement(se, se2);
      r = MultiHitOrMiss(img, 1, &se, &se2, &out);
      ImageGrayDelete(se2);
   }
   else
   {
      fprintf(stderr, "Unknown op '%s'\n", opname);
      ImageGrayDelete(se);
      ImageGrayDelete(out);
      ImageGrayDelete(img);
      return(-1);
   }
   if (r)  fprintf(stderr, "MultiErosion returned error %d!\n", r);
   r = ImageGrayPGMBinWrite(se, "se.pgm");
   r = ImageGrayPGMBinWrite(out, outfname);
   if (r)  fprintf(stderr, "Error (%d) writing image '%s'\n", r, outfname);
   ImageGrayDelete(se);
   ImageGrayDelete(out);
   ImageGrayDelete(img);
   return(0);
} /* main */
