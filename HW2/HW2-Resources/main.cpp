//Image Manipulation Skeleton Code
//
//
//  main.c
//  original by Wagner Correa, 1999
//  modified by Robert Osada, 2000
//  modified by Renato Werneck, 2003
//  modified by Stephen J. Guy, 2010-2025

#include "image.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>


#define STB_IMAGE_IMPLEMENTATION //only place once in one .cpp file
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION //only place once in one .cpp files
#include "stb_image_write.h"


using namespace std;


/**
 * prototypes
 **/
static void ShowUsage(void);
static void CheckOption(char *option, int argc, int minargc);

int main( int argc, char* argv[] ){
	Image *img = NULL;
	bool did_output = false;

	// first argument is program name
	argv++, argc--;

	// look for help
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-help")) {
			ShowUsage();
		}
	}

	// no argument case
	if (argc == 0) {
		ShowUsage();
	}

	// parse arguments
	while (argc > 0){
		if (**argv == '-'){
			if (!strcmp(*argv, "-input")){
				CheckOption(*argv, argc, 2);
				if (img != NULL)
					delete img;
				img = new Image(argv[1]);
				argv += 2, argc -= 2;
			}
			else if (!strcmp(*argv, "-output")){
				CheckOption(*argv, argc, 2);
				if (img == NULL) ShowUsage();
				img->Write(argv[1]);
				did_output = true;
				argv += 2, argc -= 2;
			}
			else if (!strcmp(*argv, "-brightness")){
				double factor;
				CheckOption(*argv, argc, 2);
				if (img == NULL) ShowUsage();

				factor = atof(argv[1]);
				img->Brighten(factor);
				argv += 2, argc -=2;
			}
			else if (!strcmp(*argv, "-crop")){
				int x, y, w, h;
				CheckOption(*argv, argc, 5);
				if (img == NULL) ShowUsage();

				x = atoi(argv[1]);
				y = atoi(argv[2]);
				w = atoi(argv[3]);
				h = atoi(argv[4]);

				Image *dst = img->Crop(x, y, w, h);
				delete img;
				img = dst;

				argv += 5, argc -= 5;
			}
			else if (!strcmp(*argv, "-extractChannel")){
				int channel;
				CheckOption(*argv, argc, 2);
				if (img == NULL) ShowUsage();

				channel = atoi(argv[1]);
				img->ExtractChannel(channel);
				argv += 2, argc -= 2;
			}
			else if (!strcmp(*argv, "-quantize")){
				int nbits;
				CheckOption(*argv, argc, 2);
				if (img == NULL) ShowUsage();

				nbits = atoi(argv[1]);
				img->Quantize(nbits);
				argv += 2, argc -= 2;
			}
			else if (!strcmp(*argv, "-ppm_depth")){
				int bits;
				CheckOption(*argv, argc, 2);
				if (img == NULL) ShowUsage();

				bits = atof(argv[1]);
				img->export_depth = bits;
				argv += 2, argc -=2;
			}
			else{
				fprintf(stderr, "image: invalid option: %s\n", *argv);
				ShowUsage();
			}
		} 
		else {
			fprintf(stderr, "image: invalid option: %s\n", *argv);
			ShowUsage();
		}
	}

	if (!did_output){
		fprintf( stderr, "WARNING: No output specified!\n" );
	}

	delete img;
	return EXIT_SUCCESS;
}


/**
 * ShowUsage
 **/
static char options[] =
"-help\n"
"-input <file>\n"
"-output <file>\n"
"-crop <x> <y> <width> <height>\n"
"-brightness <factor>\n"
"-extractChannel <channel no>\n"
"-quantize <nbits>\n"
"-ppm_depth <nbits>\n"
;

static void ShowUsage(void)
{
	fprintf(stderr, "Usage: image -input <filename>  -output <filename>\n");
	fprintf(stderr, "%s", options);
	exit(EXIT_FAILURE);
}


/**
 * CheckOption
 **/
static void CheckOption(char *option, int argc, int minargc){
	if (argc < minargc){
		fprintf(stderr, "Too few arguments for %s\n", option);
		ShowUsage();
	}
}