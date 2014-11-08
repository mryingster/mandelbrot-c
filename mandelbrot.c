// -*- compile-command: "gcc -o mandelbrot mandelbrot.c -lgd -lpng -Wall" -*-
// Copyright (c) 2014 Michael Caldwell

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gd.h>
#include <string.h>
#include <err.h>

void save_png(gdImagePtr im, const char *filename)
{
    FILE *fp;
    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Can't save png image %s\n", filename);
        return;
    }
    gdImagePng(im, fp);
    fclose(fp);
}

void help()
{
    //      0        10        20        30        40        50        60        70        80
    //      |         |         |         |         |         |         |         |         |
    printf("Mandelbrot\n\n"
           "Usage\n"
           "    Generates mandelbrot images based on user parameters.\n\n"
           "Options\n"
           "    -h, --help      Show help screen\n"
           "    -o <output.png> Specify output PNG filename   (default: mandelbrot.png)\n"
           "    --width <int>   Specify image width in pixels (default: 1024)\n"
           "    --height <int>  Specify image height in pixels (default: 1024)\n"
           "    --coords <x> <y> <x range> <y range>\n"
           "                    Specify coordinates for view (default: -2 2 4 4)\n"
           "    --gradient <hex> <hex>\n"
           "                    Specify gradient starting and ending colors in 32 bit HEX\n"
           "                    (default: 0x0000FF 0xFF0000)\n"
           "    --depth <int>   Specify how many times to calculate each pixel\n"
           "                    (default: 50)\n");
    exit(0);
}

int hex_color(int r, int g, int b)
{
  return ( r << 16 & 0xff0000 ) | ( g << 8 & 0x00ff00 ) | ( b & 0x0000ff );
}

int return_color(int color1, int color2, int depth, int z)
{
    int rStart = color1 >> 16, rEnd = color2 >> 16;
    float rStep = (rEnd - rStart) / depth;
    int gStart = color1 >> 8 & 0xff, gEnd = color2 >> 8 & 0xff;
    float gStep = (gEnd - gStart) / depth;
    int bStart = color1 & 0xff, bEnd = color2 & 0xff;
    float bStep = (bEnd - bStart) / depth;
    return hex_color(rStart+(rStep*z), gStart+(gStep*z), bStart+(bStep*z));
}

int output_gd_png(void *_array, int width, int height, int depth, const char *filename, int colorStart, int colorEnd)
{
    int x, y;
    int (*array)[height][width] = _array;
    int color=0;

    // Draw to buffer
    gdImagePtr im;
    im = gdImageCreateTrueColor(width, height);

    for (y=0; y<height; y++)
        for (x=0; x<width; x++)
        {
            if ((*array)[y][x] == -1)
                color=0;
            else
                color=return_color(colorStart, colorEnd, depth, (*array)[y][x]);
            gdImageSetPixel(im, x, y, color);
        }

    // Write to file
    save_png(im, filename);
    gdImageDestroy(im);
    return 0;
}

int mandel(double x, double y, int depth)
{
    double xP=0, yP=0; // Prime Values
    double xT=0, yT=0; // Temporary Values
    int i=0;

    for (i=0; i<depth; i++)
    {
        xT = (pow(xP, 2)) + x - (pow(yP, 2));
        yT = 2 * xP * yP + y;
        if (pow(fabs(xT),2) + pow(fabs(yT),2) > 4)
            return i;
        xP = xT;
        yP = yT;
    }
    return -1;
}

int arg_check_int(int argc, char *argv[], int i, int numvar, int base)
{
    char *endptr;
    int  return_value;
    if (i == argc-numvar) errx(EXIT_FAILURE, "Argument, \"%s,\" requires additional %s.", argv[i], base == 10 ? "integer" : base == 16 ? "HEX number" : "base %i number");
    return_value = strtoul(argv[i+numvar], &endptr, base);
    if (*endptr || return_value < 1)
        errx(EXIT_FAILURE, "Bad argument, \"%s\". Must specify a positive %s.", argv[i], base == 10 ? "integer" : base == 16 ? "HEX number" : "base %i number");
    return return_value;
}

float arg_check_float(int argc, char *argv[], int i, int numvar)
{
    char *endptr;
    double return_value;
    if (i == argc-numvar) errx(EXIT_FAILURE, "Argument, \"%s,\" requires additional floating point number.", argv[i]);
    return_value = strtod(argv[i+numvar], &endptr);
    if (*endptr)
        errx(EXIT_FAILURE, "Bad argument, \"%s\". Must specify a floating point number.", argv[i]);
    return return_value;
}

int main(int argc, char *argv[])
{
    double xStart = -2,   yStart = 2;    // Default Start Coordinates
    double xRange = 4,    yRange = 4;    // Default Range Coordinates
    int    width  = 1024, height = 1024; // Default Pixel size of output
    int    depth  = 50;                  // Default Depth level of Mandelbrot Calculation
    int    colorStart = 0x0000FF, colorEnd = 0xFF0000; // Default color gradient settings
    char   *filename = "mandelbrot.png"; // Default PNG output name
    int    i;                            // Misc variables

    // Parse Arguments
    for (i=1; i<argc; i++)
     {
        if (strcmp(argv[i], "--width") == 0)
        {
            width = arg_check_int(argc, argv, i, 1, 10);
            i++;
        }
        else if (strcmp(argv[i], "--height") == 0)
        {
            height = arg_check_int(argc, argv, i, 1, 10);
            i++;
        }
        else if (strcmp(argv[i], "--depth") == 0)
        {
            depth = arg_check_int(argc, argv, i, 1, 10);
            i++;
        }
        else if (strcmp(argv[i], "--coords") == 0)
        {
            xStart = arg_check_float(argc, argv, i, 1);
            yStart = arg_check_float(argc, argv, i, 2);
            xRange = arg_check_float(argc, argv, i, 3);
            yRange = arg_check_float(argc, argv, i, 4);
            i+=4;
        }
        else if (strcmp(argv[i], "--gradient") == 0)
        {
            colorStart = arg_check_int(argc, argv, i, 1, 16);
            colorEnd = arg_check_int(argc, argv, i, 2, 16);
            i+=2;
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            filename = argv[i+1];
            i++;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 )
        {
            help();
        }
        else
        {
            errx(EXIT_FAILURE, "Unknown argument, \"%s\".", argv[i]);
        }
     }

    double xStep  = xRange/width;        // X Step Value
    double yStep  = yRange/height;       // Y Step Value
    double xValue, yValue;               // X and Y Values for Pixel Locations
    int    x, y;                         // X and Y values for table
    int    (*array)[height][width] = malloc(height*width*sizeof(int)); // Array to store values

    //printf("%f %f %f %f %d %d %f %f\n", xStart, yStart, xRange, yRange, width, height, xStep, yStep);

    // Main loop
    for (y=0; y<height; y++)
    {
        for (x=0; x<width; x++)
        {
            xValue = xStart + (x * xStep);
            yValue = yStart - (y * yStep);
            (*array)[y][x] = mandel(xValue, yValue, depth);
        }
        printf("\r%i%% Complete", ((y+1)*100)/height);
        fflush(stdout);
    }

    // Output
    printf("\nWriting to file.\n");
    output_gd_png(*array, width, height, depth, filename, colorStart, colorEnd);

    // Exit
    free(*array);
    return 0;
}
