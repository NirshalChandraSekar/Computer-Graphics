//CSCI 5607 HW 2 - Image Conversion Instructor: S. J. Guy <sjguy@umn.edu>
//In this assignment you will load and convert between various image formats.
//Additionally, you will manipulate the stored image data by quantizing, cropping, and suppressing channels

#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include <fstream>
using namespace std;

//TODO - HW2: The current implementation of write_ppm ignores the paramater "bits" and assumes we want to write out an 8-bit PPM ...
//TODO - HW2: ... you need to adjust the function to scale the values written in the PPM file based on the "bits" variable
void write_ppm(char* imgName, int width, int height, int bits, const uint8_t *data){
   //Open the texture image file
   ofstream ppmFile;
   ppmFile.open(imgName);
   if (!ppmFile){
      printf("ERROR: Could not create file '%s'\n",imgName);
      exit(1);
   }

   //Set this as an ASCII PPM (first line is P3)
   string PPM_style = "P3\n";
   ppmFile << PPM_style; //Read the first line of the header    

   //Write out the texture width and height
   ppmFile << width << " "  << height << "\n" ;

   //Set's the 3rd line to 255 (ie., assumes this is an 8 bit/pixel PPM)
   //TODO - HW2: Set the maximum values based on the value of the variable 'bits'
   int maximum = 255;
   ppmFile << maximum << "\n" ;

   //TODO - HW2: The values in data are all 8 bits, you must convert down to whatever the variable bits is when writing the file
   int r, g, b, a;
   for (int i = 0; i < height; i++){
      for (int j = 0; j < width; j++){
		  r = data[i*width*4 + j*4 + 0];  //Red
        g = data[i*width*4 + j*4 + 1];  //Green
        b = data[i*width*4 + j*4 + 2];  //Blue
        a = data[i*width*4 + j*4 + 3];  //Alpha
        ppmFile << r << " " << g << " "  << b << " " ;
      }
   }

   ppmFile.close();
}

//TODO - HW2: The current implementation of read_ppm() assumes the PPM file has a maximum value of 255 (ie., an 8-bit PPM) ...
//TODO - HW2: ... you need to adjust the function to support PPM files with a max value of 1, 3, 7, 15, 31, 63, 127, and 255 (why these numbers?)
uint8_t* read_ppm(char* imgName, int& width, int& height){
   //Open the texture image file
   ifstream ppmFile;
   ppmFile.open(imgName);
   if (!ppmFile){
      printf("ERROR: Image file '%s' not found.\n",imgName);
      exit(1);
   }

   //Check that this is an ASCII PPM (first line is P3)
   string PPM_style;
   ppmFile >> PPM_style; //Read the first line of the header    
   if (PPM_style != "P3") {
      printf("ERROR: PPM Type number is %s. Not an ASCII (P3) PPM file!\n",PPM_style.c_str());
      exit(1);
   }

   //Read in the texture width and height
   ppmFile >> width >> height;
   unsigned char* img_data = new unsigned char[4*width*height];

   //Check that the 3rd line is 255 (ie., this is an 8 bit/pixel PPM)
   int maximum;
   ppmFile >> maximum;
   //TODO - HW2: Remove this check below, instead make the function for for maximum values besides 255
   if (maximum != 255) {
      printf("ERROR: We assume Maximum size is 255, not (%d)\n",maximum);
      printf("TODO: This error means you didn't finish your HW yet!\n");
      exit(1);
   }
	
   //TODO - HW2: The values read from the file might not be 8-bits (ie, a maximum values besides 255)
   //TODO - HW2: However img_data stores all values as 8-bit integers.
   //TODO - HW2: When you read the values into img_data scale the values up to be 8 bits
   //TODO - HW2: For example, the value 1 in a 1 bit PPM should become 255 ... 
   //TODO - HW2: Likewise, the value 1 in a 2 bit PPM should become 127 (or 128).
   int r, g, b;
   for (int i = 0; i < height; i++){
      for (int j = 0; j < width; j++){
         ppmFile >> r >> g >> b;
         img_data[i*width*4 + j*4] = r;  //Red
         img_data[i*width*4 + j*4 + 1] = g;  //Green
         img_data[i*width*4 + j*4 + 2] = b;  //Blue
         img_data[i*width*4 + j*4 + 3] = 255;  //Alpha
      }
   }

   return img_data;
}

/**
 * Image
 **/
Image::Image (int width_, int height_){

    assert(width_ > 0);
    assert(height_ > 0);

    width           = width_;
    height          = height_;
    num_pixels      = width * height;
    sampling_method = IMAGE_SAMPLING_POINT;
    
    data.raw = new uint8_t[num_pixels*4];
		int b = 0; //which byte to write to
		for (int j = 0; j < height; j++){
			for (int i = 0; i < width; i++){
				data.raw[b++] = 0;
				data.raw[b++] = 0;
				data.raw[b++] = 0;
				data.raw[b++] = 0;
			}
		}

    assert(data.raw != NULL);
}

Image::Image (const Image& src){
	width           = src.width;
	height          = src.height;
	num_pixels      = width * height;
	sampling_method = IMAGE_SAMPLING_POINT;
	
	data.raw = new uint8_t[num_pixels*4];
	
	//memcpy(data.raw, src.data.raw, num_pixels);
	*data.raw = *src.data.raw;
}

Image::Image (char* fname){

	int lastc = strlen(fname);
	if (string(fname+lastc-3) == "ppm"){
		data.raw = read_ppm(fname, width, height);
	}
	else{
		int numComponents; //(e.g., Y, YA, RGB, or RGBA)
		data.raw = stbi_load(fname, &width, &height, &numComponents, 4);
	}
	
	if (data.raw == NULL){
		printf("Error loading image: %s", fname);
		exit(-1);
	}
	
	num_pixels = width * height;
	sampling_method = IMAGE_SAMPLING_POINT;
	
}

Image::~Image (){
    delete data.raw;
    data.raw = NULL;
}

void Image::Write(char* fname){
	
	int lastc = strlen(fname);

	switch (fname[lastc-1]){
		 case 'm': //ppm
		 	 write_ppm(fname, width, height, export_depth, data.raw);
				break;
	   case 'g': //jpeg (or jpg) or png
	     if (fname[lastc-2] == 'p' || fname[lastc-2] == 'e') //jpeg or jpg
	        stbi_write_jpg(fname, width, height, 4, data.raw, 95);  //95% jpeg quality
	     else //png
	        stbi_write_png(fname, width, height, 4, data.raw, width*4);
	     break;
	   case 'a': //tga (targa)
	     stbi_write_tga(fname, width, height, 4, data.raw);
	     break;
	   case 'p': //bmp
	   default:
	     stbi_write_bmp(fname, width, height, 4, data.raw);
	}
}


//TODO - HW2: Ok, not much to do here, but read through this carefully =)
//TODO - HW2: In particular, make sure you understand how GetPixel() works, I use it two different ways here!
void Image::Brighten (double factor){
	int x,y;
	for (x = 0 ; x < Width() ; x++){
		for (y = 0 ; y < Height() ; y++){
			Pixel p = GetPixel(x, y);
			Pixel scaled_p = p*factor;
			GetPixel(x,y) = scaled_p;
		}
	}
}


//TODO - HW2: Crop an image to a rectangle starting at (x,y) with a width w and a height h
Image* Image::Crop(int x, int y, int w, int h) {
	/* WORK HERE */
	return NULL;
}

//TODO - HW2: Keep only non-zero red, green, or blue components for the channel value 0, 1, and 2 respectively
void Image::ExtractChannel(int channel) {
	/* WORK HERE */
}

//TODO - HW2: Quantize the intensities stored for each pixel's values into 2^nbits possible equally-spaced values
//TODO - HW2: You may find a very helpful function in the pixel class!
void Image::Quantize (int nbits) {
	/* WORK HERE */
}
