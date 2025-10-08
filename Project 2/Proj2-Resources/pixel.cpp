#include "pixel.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>



/**
 * Component Operations
 **/
Component ComponentRandom(void){
    return rand() % 256;
}

Component ComponentScale(Component c, double f){
    return ComponentClamp((int) floor(c * f + 0.5));
}

Component ComponentLerp(Component c, Component d, double t){
    return ComponentClamp((int) floor((1.0 - t) * c + t * d + 0.5));
}



/**
 * Pixel Operations
 **/
// Compute the luminance of the pixel (perceptual brightness) [ITU-R 601-2 standard]
Component Pixel::Luminance (){
    return (r * 76 + g * 150 + b * 29) >> 8;
}

// Set the pixel values, clamping to [0,255]
void Pixel::SetClamp (double r_, double g_, double b_){
    r = ComponentClamp((int)r_);
    g = ComponentClamp((int)g_);
    b = ComponentClamp((int)b_);
}

// Set the pixel values, clamping to [0,255]
void Pixel::SetClamp (double r_, double g_, double b_, double a_){
    r = ComponentClamp((int)r_);
    g = ComponentClamp((int)g_);
    b = ComponentClamp((int)b_);
    a = ComponentClamp((int)a_);
}

// Generate a random pixel
Pixel PixelRandom(void){
    return Pixel(
        ComponentRandom(),
        ComponentRandom(),
        ComponentRandom(),
        ComponentRandom());
}

// Component-wise addition of two pixels rgba values
Pixel operator+ (const Pixel& p, const Pixel& q){
    return Pixel(
        ComponentClamp(p.r + q.r),
        ComponentClamp(p.g + q.g),
        ComponentClamp(p.b + q.b),
        ComponentClamp(p.a + q.a));
}

// Component-wise multiplication of two pixel rgba values
Pixel operator* (const Pixel& p, const Pixel& q){
    return Pixel(
        ComponentClamp(p.r * q.r),
        ComponentClamp(p.g * q.g),
        ComponentClamp(p.b * q.b),
        ComponentClamp(p.a * q.a));
}


// Scale a pixel by a scalar factor
Pixel operator* (const Pixel& p, double f){
    return Pixel(
        ComponentScale(p.r, f),
        ComponentScale(p.g, f),
        ComponentScale(p.b, f),
        ComponentScale(p.a, f));
}


// Linear interpolation between two pixel rgba values 
Pixel PixelLerp (const Pixel& p, const Pixel& q, double t){
    return Pixel(
        ComponentLerp(p.r, q.r, t),
        ComponentLerp(p.g, q.g, t),
        ComponentLerp(p.b, q.b, t),
        ComponentLerp(p.a, q.a, t));
}


// Quantize a pixel to nbits per channel (nbits <= 8)
Pixel PixelQuant( const Pixel &p, int nbits){
	int shift = 8-nbits;
	float mult = 255/float(255 >> shift);
	int new_r, new_g, new_b;
	new_r = (p.r >> shift);
	new_g = (p.g >> shift);
	new_b = (p.b >> shift);

	Pixel ret;
	ret.SetClamp(new_r*mult , new_g*mult , new_b*mult );
	return ret;
}