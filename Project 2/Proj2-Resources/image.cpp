//CSCI 5607 HW 2 - Image Conversion Instructor: S. J. Guy <sjguy@umn.edu>
//In this assignment you will load and convert between various image formats.
//Additionally, you will manipulate the stored image data by quantizing, cropping, and suppressing channels

#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <random>
#include <algorithm>
#include <fstream>
using namespace std;

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

Image::Image(const Image& src){
	width           = src.width;
	height          = src.height;
	num_pixels      = width * height;
	sampling_method = IMAGE_SAMPLING_POINT;
	
	data.raw = new uint8_t[num_pixels*sizeof(Pixel)];
	
	memcpy(data.raw, src.data.raw, num_pixels*sizeof(Pixel));
}

Image::Image(char* fname){

	int numComponents; //(e.g., Y, YA, RGB, or RGBA)

	//Load the pixels with STB Image Lib
	uint8_t* loadedPixels = stbi_load(fname, &width, &height, &numComponents, 4);
	if (loadedPixels == NULL){
		printf("Error loading image: %s", fname);
		exit(-1);
	}

	//Set image member variables
	num_pixels = width * height;
	sampling_method = IMAGE_SAMPLING_POINT;

  //Copy the loaded pixels into the image data structure
	data.raw = new uint8_t[num_pixels*sizeof(Pixel)];
	memcpy(data.raw, loadedPixels, num_pixels*sizeof(Pixel));
	free(loadedPixels);
}

Image::~Image(){
    delete[] data.raw;
    data.raw = NULL;
}

void Image::Write(char* fname){
	
	int lastc = strlen(fname);

	switch (fname[lastc-1]){
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

void Image::ExtractChannel(int channel){
	/* WORK HERE */
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


void Image::Quantize (int nbits){
	/* WORK HERE */
	assert(nbits >= 1 && nbits <= 8); // validate bits

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            Pixel p = GetPixel(i, j);
            Pixel q = PixelQuant(p, nbits);  // quantize using PixelQuant
            GetPixel(i, j) = q;              // assign back to image
        }
    }
}

Image* Image::Crop(int x, int y, int w, int h){
	if (!ValidCoord(x, y)) {
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


void Image::AddNoise (double factor){
	/* WORK HERE */
	int num_channels = 3; //RGB
	float mean = 0.0;
	float stddev = factor; //Variance is square of stddev

	default_random_engine generator;
	normal_distribution<float> distribution(mean, stddev);

	// Add noise to each channel of each pixel
	for(int j=0; j<height; j++){
		for(int i=0; i<width; i++){
			Pixel& p = GetPixel(i, j);
			int noise_r = (int)(distribution(generator));
			int noise_g = (int)(distribution(generator));
			int noise_b = (int)(distribution(generator));

			p.r = std::min(255, std::max(0, p.r + noise_r));
			p.g = std::min(255, std::max(0, p.g + noise_g));
			p.b = std::min(255, std::max(0, p.b + noise_b));
		}
	}
}

void Image::ChangeContrast (double factor){
	/* WORK HERE */

	for(int j=0; j<height; j++){
		for(int i=0; i<width; i++){
			Pixel& p = GetPixel(i, j);
			int r = 128 + factor * (p.r - 128);
			int g = 128 + factor * (p.g - 128);
			int b = 128 + factor * (p.b - 128);

			p.r = std::min(255, std::max(0, r));
			p.g = std::min(255, std::max(0, g));
			p.b = std::min(255, std::max(0, b));
		}
	}
}


void Image::ChangeSaturation(double factor){
	/* WORK HERE */

	for(int j=0; j<height; j++){
		for(int i=0; i<width; i++){
			Pixel& p = GetPixel(i, j);
			
			double r = p.r;
			double g = p.g;
			double b = p.b;

			double gray = 0.299*r + 0.587*g + 0.114*b;

			r = gray + factor * (r - gray);
			g = gray + factor * (g - gray);
			b = gray + factor * (b - gray);

			p.r = std::min(255, std::max(0, (int)r));
			p.g = std::min(255, std::max(0, (int)g));
			p.b = std::min(255, std::max(0, (int)b));
		}
	}

}


//For full credit, check that your dithers aren't making the pictures systematically brighter or darker
void Image::RandomDither (int nbits){
	/* WORK HERE */
	assert(nbits > 0 && nbits <= 8);
    
    int levels = 1 << nbits; // 2^nbits
    
    // Seed random generator once
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(nullptr));
        seeded = true;
    }

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            Pixel& p = GetPixel(i, j);

            auto dither_channel = [&](int val) -> int {
                double r = ((double)rand() / RAND_MAX); // random in [0,1)
                int q = (int)((val * levels / 256.0) + r);
                int new_val = (int)round(q * 255.0 / (levels - 1));
                return std::min(255, std::max(0, new_val));
            };

            p.r = dither_channel(p.r);
            p.g = dither_channel(p.g);
            p.b = dither_channel(p.b);
        }
    }
}

//This bayer method gives the quantization thresholds for an ordered dither.
//This is a 4x4 dither pattern, assumes the values are quantized to 16 levels.
//You can either expand this to a larger bayer pattern. Or (more likely), scale
//the threshold based on the target quantization levels.
static int Bayer4[4][4] ={
    {15,  7, 13,  5},
    { 3, 11,  1,  9},
    {12,  4, 14,  6},
    { 0,  8,  2, 10}
};


void Image::OrderedDither(int nbits){
	/* WORK HERE  (Extra Credit) */
}

/* Error-diffusion parameters */
const double
    ALPHA = 7.0 / 16.0,
    BETA  = 3.0 / 16.0,
    GAMMA = 5.0 / 16.0,
    DELTA = 1.0 / 16.0;

void Image::FloydSteinbergDither(int nbits){
	/* WORK HERE */
	assert(nbits > 0 && nbits <= 8);
    int levels = 1 << nbits; // 2^nbits

    // Create a copy of the image as float buffer to hold propagated errors
    std::vector<std::vector<std::array<double,3>>> buffer(height, std::vector<std::array<double,3>>(width));

    // Initialize buffer with original pixel values
    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            Pixel p = GetPixel(x,y);
            buffer[y][x][0] = p.r;
            buffer[y][x][1] = p.g;
            buffer[y][x][2] = p.b;
        }
    }

    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            for(int c=0; c<3; c++) { // R,G,B channels
                double oldVal = buffer[y][x][c];
                double newVal = std::round(oldVal * (levels-1) / 255.0) * 255.0 / (levels-1);
                double error = oldVal - newVal;

                buffer[y][x][c] = newVal; // store quantized value

                // Distribute error to neighbors
                if(x+1 < width)        buffer[y][x+1][c] += error * ALPHA;
                if(x-1 >= 0 && y+1<height) buffer[y+1][x-1][c] += error * BETA;
                if(y+1 < height)       buffer[y+1][x][c] += error * GAMMA;
                if(x+1 < width && y+1<height) buffer[y+1][x+1][c] += error * DELTA;
            }

            // Write back to image (clamped)
            Pixel& p = GetPixel(x,y);
            p.r = std::min(255, std::max(0, (int)std::round(buffer[y][x][0])));
            p.g = std::min(255, std::max(0, (int)std::round(buffer[y][x][1])));
            p.b = std::min(255, std::max(0, (int)std::round(buffer[y][x][2])));
        }
    }
}

// Gaussian blur with size nxn filter
void Image::Blur(int n){
   // float r, g, b; //You'll get better results converting everything to floats, then converting back to bytes (less quantization error)
	// Image* img_copy = new Image(*this); //This is will copying the image, so you can read the original values for filtering
                                          //  ... don't forget to delete the copy!
	/* WORK HERE */

	assert(n % 2 == 1); // n must be odd
	int radius = n / 2;
	double sigma = radius;

	// Create Gaussian kernel
	vector<vector<double>> kernel(n, vector<double>(n));
	double sum = 0.0;

	for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            double weight = exp(-(x * x + y * y) / (2 * sigma * sigma));
            kernel[y + radius][x + radius] = weight;
            sum += weight;
        }
    }

    // Normalize kernel so total weight = 1
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            kernel[y][x] /= sum;
        }
    }

	// Create a copy of the original image to read from
	Image* img_copy = new Image(*this);

	// Apply Gaussian blur
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            double r = 0.0, g = 0.0, b = 0.0;

            // Convolution over kernel
            for (int ky = -radius; ky <= radius; ky++) {
                for (int kx = -radius; kx <= radius; kx++) {
                    int px = std::min(width - 1, std::max(0, i + kx));
                    int py = std::min(height - 1, std::max(0, j + ky));

                    Pixel p = img_copy->GetPixel(px, py);
                    double w = kernel[ky + radius][kx + radius];
                    r += w * p.r;
                    g += w * p.g;
                    b += w * p.b;
                }
            }

            Pixel& out = GetPixel(i, j);
            out.r = (int)std::min(255.0, std::max(0.0, r));
            out.g = (int)std::min(255.0, std::max(0.0, g));
            out.b = (int)std::min(255.0, std::max(0.0, b));
        }
    }

    delete img_copy;
}

void Image::Sharpen(int n){
	/* WORK HERE */
	assert(n % 2 == 1); // ensure odd kernel size

    double factor = 2.0;  // sharpening intensity

    // Step 1: Create a blurred version
    Image* blurred = new Image(*this);
    blurred->Blur(n);

    // Step 2: Apply unsharp mask
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            Pixel& orig = GetPixel(i, j);
            Pixel blur_p = blurred->GetPixel(i, j);

            int r = (int)((1 + factor) * orig.r - factor * blur_p.r);
            int g = (int)((1 + factor) * orig.g - factor * blur_p.g);
            int b = (int)((1 + factor) * orig.b - factor * blur_p.b);

            orig.r = std::min(255, std::max(0, r));
            orig.g = std::min(255, std::max(0, g));
            orig.b = std::min(255, std::max(0, b));
        }
    }

    delete blurred;
}

void Image::EdgeDetect(){
	/* WORK HERE */
	Image* copy = new Image(*this);

    // Sobel kernels
    int Gx[3][3] = {{-1,0,1}, {-2,0,2}, {-1,0,1}};
    int Gy[3][3] = {{-1,-2,-1}, {0,0,0}, {1,2,1}};

    for (int y = 1; y < height-1; y++) {
        for (int x = 1; x < width-1; x++) {
            double sumX = 0;
            double sumY = 0;

            for (int j = -1; j <= 1; j++) {
                for (int i = -1; i <= 1; i++) {
                    Pixel p = copy->GetPixel(x+i, y+j);
                    // convert to grayscale
                    double gray = 0.299*p.r + 0.587*p.g + 0.114*p.b;
                    sumX += gray * Gx[j+1][i+1];
                    sumY += gray * Gy[j+1][i+1];
                }
            }

            // Gradient magnitude
            int val = (int)std::min(255.0, std::sqrt(sumX*sumX + sumY*sumY));
            Pixel& orig = GetPixel(x,y);
            orig.r = orig.g = orig.b = val;
        }
    }

    delete copy;
}

Image* Image::Scale(double sx, double sy){
	/* WORK HERE */
	int newW = std::round(width * sx);
    int newH = std::round(height * sy);

    // Change this to "point", "bilinear", or "gaussian"
    std::string method = "gaussian ";  

    Image* out = new Image(newW, newH);

    for (int y_new = 0; y_new < newH; y_new++) {
        for (int x_new = 0; x_new < newW; x_new++) {
            double x = x_new / sx;
            double y = y_new / sy;
            Pixel p;

            if (method == "point") {
                int xn = std::min(width - 1, std::max(0, int(std::floor(x))));
                int yn = std::min(height - 1, std::max(0, int(std::floor(y))));
                p = GetPixel(xn, yn);
            } 
            else if (method == "bilinear") {
                int x0 = std::min(width - 1, std::max(0, int(std::floor(x))));
                int y0 = std::min(height - 1, std::max(0, int(std::floor(y))));
                int x1 = std::min(width - 1, x0 + 1);
                int y1 = std::min(height - 1, y0 + 1);
                double dx = x - x0;
                double dy = y - y0;

                Pixel p00 = GetPixel(x0, y0);
                Pixel p10 = GetPixel(x1, y0);
                Pixel p01 = GetPixel(x0, y1);
                Pixel p11 = GetPixel(x1, y1);

                p.r = std::round((1 - dx) * (1 - dy) * p00.r + dx * (1 - dy) * p10.r + (1 - dx) * dy * p01.r + dx * dy * p11.r);
                p.g = std::round((1 - dx) * (1 - dy) * p00.g + dx * (1 - dy) * p10.g + (1 - dx) * dy * p01.g + dx * dy * p11.g);
                p.b = std::round((1 - dx) * (1 - dy) * p00.b + dx * (1 - dy) * p10.b + (1 - dx) * dy * p01.b + dx * dy * p11.b);
            } 
            else if (method == "gaussian") {
                double kernel[3][3] = {
                    {1, 2, 1},
                    {2, 4, 2},
                    {1, 2, 1}
                };
                double sumK = 16.0;
                double r = 0, g = 0, b = 0;

                int cx = int(std::round(x));
                int cy = int(std::round(y));

                for (int j = -1; j <= 1; j++) {
                    for (int i = -1; i <= 1; i++) {
                        int xi = std::min(width - 1, std::max(0, cx + i));
                        int yj = std::min(height - 1, std::max(0, cy + j));
                        Pixel px = GetPixel(xi, yj);
                        r += px.r * kernel[j + 1][i + 1];
                        g += px.g * kernel[j + 1][i + 1];
                        b += px.b * kernel[j + 1][i + 1];
                    }
                }

                p.r = std::round(r / sumK);
                p.g = std::round(g / sumK);
                p.b = std::round(b / sumK);
            }

            out->SetPixel(x_new, y_new, p);
        }
    }

    return out;
}

Image* Image::Rotate(double angle){
	/* WORK HERE */
	double rad = angle * M_PI / 180.0;

    // Output image same size (you can enlarge if needed)
    Image* out = new Image(width, height);

    double cx = width / 2.0;
    double cy = height / 2.0;

    for(int y_new = 0; y_new < height; y_new++) {
        for(int x_new = 0; x_new < width; x_new++) {

            // Compute source coordinates
            double x =  cos(rad)*(x_new - cx) + sin(rad)*(y_new - cy) + cx;
            double y = -sin(rad)*(x_new - cx) + cos(rad)*(y_new - cy) + cy;

            Pixel p;

            // Check bounds
            if(x >= 0 && x < width-1 && y >= 0 && y < height-1) {
                int x0 = std::floor(x);
                int y0 = std::floor(y);
                int x1 = x0 + 1;
                int y1 = y0 + 1;

                double dx = x - x0;
                double dy = y - y0;

                Pixel p00 = GetPixel(x0, y0);
                Pixel p10 = GetPixel(x1, y0);
                Pixel p01 = GetPixel(x0, y1);
                Pixel p11 = GetPixel(x1, y1);

                double r = (1-dx)*(1-dy)*p00.r + dx*(1-dy)*p10.r + (1-dx)*dy*p01.r + dx*dy*p11.r;
                double g = (1-dx)*(1-dy)*p00.g + dx*(1-dy)*p10.g + (1-dx)*dy*p01.g + dx*dy*p11.g;
                double b = (1-dx)*(1-dy)*p00.b + dx*(1-dy)*p10.b + (1-dx)*dy*p01.b + dx*dy*p11.b;

                p.r = std::round(r);
                p.g = std::round(g);
                p.b = std::round(b);
            } else {
                p.r = p.g = p.b = 0; // black background
            }

            out->SetPixel(x_new, y_new, p);
        }
    }

    return out;

}

void Image::Fun(){
	/* WORK HERE */
}

/**
 * Image Sample
 **/
void Image::SetSamplingMethod(int method){
   assert((method >= 0) && (method < IMAGE_N_SAMPLING_METHODS));
   sampling_method = method;
}


Pixel Image::Sample (double u, double v){
   /* WORK HERE */
   return Pixel();
}