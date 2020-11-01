#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <mpi.h>

#define TRUE 1
#define FALSE 0

typedef struct PPMImage {
  char M, N;
  int max, width, height;
  unsigned char *pixels;
} PPMImage;

void flipImage(PPMImage* img);
void RGBtoGray(PPMImage* img);
void smooth(PPMImage* img);

PPMImage* ImageRead(char *filename);
void ImageWrite(PPMImage *image, char *filename);
static void checkDimension(int);
static void die(char*);
static void readPPMHeader(FILE*, int*, int*);

int main(int argc, char *argv[]) {
  double start, finish;

  start = MPI_Wtime();

  PPMImage* img = ImageRead("./small/boxes_1.ppm");
  flipImage(img);
  RGBtoGray(img);
  smooth(img);
  ImageWrite(img, "test3.ppm");

  finish = MPI_Wtime();
  printf("[Sequential] Elapsed time: %e seconds\n", finish-start);
}


void flipImage(PPMImage* img){
  unsigned char temp;
  int width = img->width;
  int height = img->height;
  for(int i=0; i<height; i++){
    for(int j=0; j<(width)/2; j++){
      for(int k=0; k<3; k++){
        temp = img->pixels[i*width*3 + j*3+k];
        img->pixels[i*width*3 + j*3+k] = img->pixels[i*width*3 + (width-1-j)*3 + k];
        img->pixels[i*width*3 + (width-1-j)*3 + k] = temp;
      }
    }
  }
}

void RGBtoGray(PPMImage* img){
  unsigned char temp;
  int width = img->width;
  int height = img->height;
  for(int i=0; i<height; i++){
    for(int j=0; j<width; j++){
      int index = i*width*3 + j*3;
      temp = (img->pixels[index] + img->pixels[index+1] + img->pixels[index+2])/3;
      for(int k=0; k<3; k++)
        img->pixels[i*width*3 + j*3+k] = temp;
    }
  }
}

void smooth(PPMImage* img){
  unsigned char *temp_pixels;
  int temp = 0;
  int width = img->width;
  int height = img->height;
  temp_pixels = (unsigned char*)malloc(sizeof(unsigned char)*width*height*3);

  for(int i=0; i<height; i++){
    for(int j=0; j<width; j++){
      temp = 0;
      /* Calculate Pixels around */
      int index = i*width*3 + j*3;
      if(i != 0){
        if(j != 0)
          temp += img->pixels[index - width*3-3];
        temp += img->pixels[index - width*3];
        if(j != width-1)
          temp += img->pixels[index - width*3+3];
      }
      
      if(j != 0)
        temp = temp +(img->pixels[index-3]);
      temp = temp + (img->pixels[index]);
      if(j != width-1)
        temp = temp + (img->pixels[index+3]);

      if(i != height-1){
        if(j != 0)
          temp += img->pixels[index + width*3-3];
        temp += img->pixels[index + width*3];
        if(j != width-1)
          temp += img->pixels[index + width*3+3];
      }

      temp /= 9;
      for(int k=0; k<3; k++)
        temp_pixels[index +k] = temp % 256;
    }
  }

  for(int i=0; i<height; i++)
    for(int j=0; j<width*3; j++)
      img->pixels[i*width*3 + j] = temp_pixels[i*width*3 + j];
}

PPMImage *
ImageRead(char *filename)
{
  int width, height, num, size;
  u_char *p;

  PPMImage *image = (PPMImage *) malloc(sizeof(PPMImage));
  FILE  *fp    = fopen(filename, "r");

  if (!image) die("cannot allocate memory for new image");
  if (!fp)    die("cannot open file for reading");

  readPPMHeader(fp, &width, &height);

  size          = width * height * 3;
  image->pixels   = (u_char *) malloc(size);
  image->width  = width;
  image->height = height;

  if (!image->pixels) die("cannot allocate memory for new image");

  num = fread((void *) image->pixels, 1, (size_t) size, fp);

  if (num != size) die("cannot read image data from file");

  fclose(fp);

  return image;
}


void ImageWrite(PPMImage *image, char *filename)
{
  int num;
  int size = image->width * image->height * 3;

  FILE *fp = fopen(filename, "w");

  if (!fp) die("cannot open file for writing");

  fprintf(fp, "P6\n%d %d\n%d\n", image->width, image->height, 255);

  num = fwrite((void *) image->pixels, 1, (size_t) size, fp);

  if (num != size) die("cannot write image data to file");

  fclose(fp);
}  

static void
die(char *message)
{
  fprintf(stderr, "ppm: %s\n", message);
  exit(1);
}


/* check a dimension (width or height) from the image file for reasonability */

static void
checkDimension(int dim)
{
  if (dim < 1 || dim > 4000) 
    die("file contained unreasonable width or height");
}


/* read a header: verify format and get width and height */

static void
readPPMHeader(FILE *fp, int *width, int *height)
{
  char ch;
  int  maxval;

  if (fscanf(fp, "P%c\n", &ch) != 1 || ch != '6') 
    die("file is not in ppm raw format; cannot read");

  /* skip comments */
  ch = getc(fp);
  while (ch == '#')
    {
      do {
	ch = getc(fp);
      } while (ch != '\n');	/* read to the end of the line */
      ch = getc(fp);            /* thanks, Elliot */
    }

  if (!isdigit(ch)) die("cannot read header information from ppm file");

  ungetc(ch, fp);		/* put that digit back */

  /* read the width, height, and maximum value for a pixel */
  fscanf(fp, "%d%d%d\n", width, height, &maxval);

  if (maxval != 255) die("image is not true-color (24 bit); read failed");
  
  checkDimension(*width);
  checkDimension(*height);
}