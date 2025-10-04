//CSCI 5607 HW 2 - Image Conversion Instructor: S. J. Guy <sjguy@umn.edu>
//In this assignment you will load and convert between various image formats.
//Additionally, you will manipulate the stored image data by quantizing, cropping, and suppressing channels

#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <cmath>

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
   int maximum = (int)pow(2, bits) - 1; // 2^bits - 1
   ppmFile << maximum << "\n" ;

   //TODO - HW2: The values in data are all 8 bits, you must convert down to whatever the variable bits is when writing the file
   int r, g, b, a;
   for (int i = 0; i < height; i++){
      for (int j = 0; j < width; j++){
		  r = data[i*width*4 + j*4 + 0];  //Red
        g = data[i*width*4 + j*4 + 1];  //Green
        b = data[i*width*4 + j*4 + 2];  //Blue

        r = (round)((r / 255.0) * maximum);
        g = (round)((g / 255.0) * maximum);
        b = (round)((b / 255.0) * maximum);
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
	
   //TODO - HW2: The values read from the file might not be 8-bits (ie, a maximum values besides 255)
   //TODO - HW2: However img_data stores all values as 8-bit integers.
   //TODO - HW2: When you read the values into img_data scale the values up to be 8 bits
   //TODO - HW2: For example, the value 1 in a 1 bit PPM should become 255 ... 
   //TODO - HW2: Likewise, the value 1 in a 2 bit PPM should become 127 (or 128).

   int r, g, b;
   float bucket_size = 256.0 / (maximum + 1); // +1 to avoid division by zero
   for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
         ppmFile >> r >> g >> b;

         // Scale from [0, maximum] → [0, 255] using "middle of bucket"
         // middle = (value * bucket_size) + (bucket_size / 2) - 1
         int r_scaled = (int)(r * bucket_size + bucket_size / 2.0f - 1);
         int g_scaled = (int)(g * bucket_size + bucket_size / 2.0f - 1);
         int b_scaled = (int)(b * bucket_size + bucket_size / 2.0f - 1);

         // Clamp to valid 0–255 range
         r_scaled = std::max(0, std::min(255, r_scaled));
         g_scaled = std::max(0, std::min(255, g_scaled));
         b_scaled = std::max(0, std::min(255, b_scaled));

         img_data[i*width*4 + j*4 + 0] = (uint8_t)r_scaled;
         img_data[i*width*4 + j*4 + 1] = (uint8_t)g_scaled;
         img_data[i*width*4 + j*4 + 2] = (uint8_t)b_scaled;
         img_data[i*width*4 + j*4 + 3] = 255;  // Alpha always 255
      }
   }
   ppmFile.close();
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
	// Make sure the crop area is inside the image
    if (x < 0 || y < 0 || x + w > width || y + h > height) {
        printf("ERROR: Crop region is out of image bounds!\n");
        return nullptr;
    }

    // Create a new image of size w × h
    Image* cropped = new Image(w, h);

    // Copy pixels from original image into new image
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            // Compute source pixel position
            Pixel p = GetPixel(x + i, y + j);

            // Assign pixel to cropped image
            cropped->GetPixel(i, j) = p;
        }
    }

    return cropped;
}

//TODO - HW2: Keep only non-zero red, green, or blue components for the channel value 0, 1, and 2 respectively
void Image::ExtractChannel(int channel) {
	if (channel < 0 || channel > 2) {
        printf("ERROR: Invalid channel %d! Must be 0 (red), 1 (green), or 2 (blue).\n", channel);
        return;
    }

    for (int y = 0; y < Height(); y++) {
        for (int x = 0; x < Width(); x++) {
            Pixel& p = GetPixel(x, y);
            if (channel == 0) { // Red
                p.g = 0;
                p.b = 0;
            } else if (channel == 1) { // Green
                p.r = 0;
                p.b = 0;
            } else if (channel == 2) { // Blue
                p.r = 0;
                p.g = 0;
            }
        }
    }
}

//TODO - HW2: Quantize the intensities stored for each pixel's values into 2^nbits possible equally-spaced values
//TODO - HW2: You may find a very helpful function in the pixel class!
void Image::Quantize (int nbits) {
	assert(nbits >= 1 && nbits <= 8); // validate bits

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            Pixel p = GetPixel(i, j);
            Pixel q = PixelQuant(p, nbits);  // quantize using PixelQuant
            GetPixel(i, j) = q;              // assign back to image
        }
    }
}
