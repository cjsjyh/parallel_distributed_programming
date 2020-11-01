#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <mpi.h>

#define TRUE 1
#define FALSE 0

typedef struct PPMImage {
  int width, height;
  unsigned char *pixels;
} PPMImage;

void flipImage(PPMImage* img);
void RGBtoGray(PPMImage* img);
void smooth(PPMImage* img, int, int, int);

PPMImage* ImageRead(char *filename);
void ImageWrite(PPMImage *image, char *filename);
static void checkDimension(int);
static void die(char*);
static void readPPMHeader(FILE*, int*, int*);

int main(int argc, char *argv[]) {
  int numprocs, rank, namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  double start, finish;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Initialize
  MPI_Status status;
  MPI_Datatype cut_pixels;
  MPI_Datatype cut_pixels_pd_1;
  MPI_Datatype cut_pixels_pd_2;

  int height, height_1, height_2 , width;
  PPMImage* img;
  PPMImage** cut_images = (PPMImage**)malloc(sizeof(PPMImage*)*numprocs);
  
  // --------------------
  // root process
  // --------------------
  if(rank == 0){
    start = MPI_Wtime();
    // Read image
    img = ImageRead("./large/start_1.ppm");
    // Declare MPI contiguous data type
    height = img->height / numprocs;
    height_1 = height + 1;
    height_2 = height + 2;
    width = img->width;
    MPI_Type_contiguous(height*width*3, MPI_UNSIGNED_CHAR, &cut_pixels);
    MPI_Type_contiguous((height+1)*width*3, MPI_UNSIGNED_CHAR, &cut_pixels_pd_1);
    MPI_Type_contiguous((height+2)*width*3, MPI_UNSIGNED_CHAR, &cut_pixels_pd_2);
    MPI_Type_commit(&cut_pixels);
    MPI_Type_commit(&cut_pixels_pd_1);
    MPI_Type_commit(&cut_pixels_pd_2);

    // Send cut images and metadata
    for(int i=1; i<numprocs; i++){
      //First section. send from row to row+1
      if(i == 1){
        MPI_Send(&height_1, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&(img->pixels[height*(i-1)*width*3]),1, cut_pixels_pd_1, i, 0, MPI_COMM_WORLD);
      }
      //Middle sections. send from row -1 to row +1
      else{
        MPI_Send(&height_2, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&(img->pixels[height*(i-1)*width*3 - width*3]),1, cut_pixels_pd_1, i, 0, MPI_COMM_WORLD);
      }
    }

    // Prepare receive buffers
    for(int i=0; i<numprocs-1; i++) {
      cut_images[i] = (PPMImage*)malloc(sizeof(PPMImage));
      cut_images[i]->width  = width;
      cut_images[i]->height = height;
      cut_images[i]->pixels = (unsigned char*) malloc(width * height * 3);
    }

    // Copy last cut image
    cut_images[numprocs-1] = (PPMImage*)malloc(sizeof(PPMImage));
    cut_images[numprocs-1]->width = width;
    cut_images[numprocs-1]->height = (img->height) - height * (numprocs-1) + 1;
    cut_images[numprocs-1]->pixels = (unsigned char*)malloc((img->height + 1)*width*3 - (numprocs-1)*height*width*3);
    // row -1 to row
    for(int i=0; i<( (img->height+1) - (numprocs-1)*height )*width*3; i++)
      cut_images[numprocs-1]->pixels[i] = img->pixels[(numprocs-1)*height*width*3 + -width*3 + i];
    // Process last cut image
    flipImage(cut_images[numprocs-1]);
    RGBtoGray(cut_images[numprocs-1]);
    smooth(cut_images[numprocs-1], 1, 0, rank);

    // Receive processed images from remote
    unsigned char *temp = (unsigned char*)malloc(sizeof(unsigned char)*height_2*width*3);
    for(int i=1; i<numprocs; i++){
      // first section
      if(i==1){
        MPI_Recv(temp, 1, cut_pixels_pd_1, i, 0, MPI_COMM_WORLD, &status);
        for(int j=0; j<height*width*3; j++)
          cut_images[i-1]->pixels[j] = temp[j];
      }
      else{
        MPI_Recv(temp, 1, cut_pixels_pd_2, i, 0, MPI_COMM_WORLD, &status);
        for(int j=0; j<height*width*3; j++)
          cut_images[i-1]->pixels[j] = temp[width*3 + j];
      }
    }

    char temp_name[10] = "test_0.ppm";
    for(int i=0; i<numprocs; i++){
      temp_name[5] = i + '0';
      ImageWrite(cut_images[i], temp_name);
    }

    // Combine images into one
    int start = 0;
    int last=-1;
    for(int i=0; i<numprocs; i++){
      int pixel_count = (cut_images[i]->height) * cut_images[i]->width * 3;
      for(int j=0; j<pixel_count; j++){
        img->pixels[start+j] = cut_images[i]->pixels[j];
        last = j+start;
      }
      start += pixel_count;
    }
    
    ImageWrite(img, "final.ppm");

    finish = MPI_Wtime();
    printf("[Parallel] Elapsed time: %e seconds\n", finish-start);
  }
  // --------------------
  // other processes
  // --------------------
  else {
    MPI_Recv(&height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&width, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    img = (PPMImage *) malloc(sizeof(PPMImage));
    img->width  = width;
    img->height = height;
    img->pixels   = (unsigned char*) malloc(width * height * 3);
    //printf("[%d] height: %d width: %d\n", rank, height, width);
    if(rank == 1){
      MPI_Type_contiguous(height*width*3, MPI_UNSIGNED_CHAR, &cut_pixels_pd_1);
      MPI_Type_commit(&cut_pixels_pd_1);
      MPI_Recv(img->pixels, 1, cut_pixels_pd_1, 0, 0, MPI_COMM_WORLD, &status);
    }
    else {
      MPI_Type_contiguous(height*width*3, MPI_UNSIGNED_CHAR, &cut_pixels_pd_2);
      MPI_Type_commit(&cut_pixels_pd_2);
      MPI_Recv(img->pixels, 1, cut_pixels_pd_2, 0, 0, MPI_COMM_WORLD, &status);

    }

    flipImage(img);
    RGBtoGray(img);
    // if(rank == 1)
    //   smooth(img, 0, 1, rank);
    // else
    //   smooth(img, 1, 1, rank);

    if(rank == 1)
      MPI_Send(img->pixels, 1, cut_pixels_pd_1, 0, 0, MPI_COMM_WORLD);
    else
      MPI_Send(img->pixels, 1, cut_pixels_pd_2, 0, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();
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

void smooth(PPMImage* img, int padding_top, int padding_bottom, int rank){
  unsigned char *temp_pixels;
  int temp = 0;
  int width = img->width;
  int height = img->height;
  temp_pixels = (unsigned char*)malloc(sizeof(unsigned char)*width*height*3);

  for(int i=0; i<height; i++){
    //printf("[%d] height i: %d\n", rank, i);
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
      else {
        //printf("[%d] LAST ROW i: %d height: %d", rank, i, height);

      }

      int divisor = 9;
      if(i == 0)  divisor -= 3;
      else if(i == height-1) divisor -= 3;
      temp /= divisor;
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