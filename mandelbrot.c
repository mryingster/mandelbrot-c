// -*- compile-command: "gcc -std=c99 -o mandelbrot mandelbrot.c -lgd -lpng $(pkg-config --libs --cflags sdl2) -Wall -O3" -*-
// Copyright (c) 2014 Michael Caldwell

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gd.h>
#include <string.h>
#include <err.h>
#include <time.h>
#include <stdlib.h>
#include <SDL.h>

typedef struct color color;

struct color{
    int r;
    int g;
    int b;
    int hex;
};

typedef struct coordinates coordinates;
struct coordinates{
    int    width;  // Width of render
    int    height; // Height of render
    double x;      // x coordinate
    double xR;     // x range
    double xS;     // x Step Value
    double y;      // y coordinate
    double yR;     // y range
    double yS;     // y step value
};

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
           "    --spectrum      Use whole RGB spectrum instead of a 2 point gradient\n"
           "    --random        Use random color values instead of gradient"
           "    --depth <int>   Specify how many times to calculate each pixel\n"
           "                    (default: 100)\n");
    exit(0);
}

color hexToColor(int hex)
{
    color c;
    c.r = hex >> 16 & 0xff;
    c.g = hex >> 8 & 0xff;
    c.b = hex & 0xff;
    c.hex = hex;
    return c;
}

int colorToHex(color c)
{
    return ( c.r << 16 & 0xff0000 ) | ( c.g << 8 & 0x00ff00 ) | ( c.b & 0x0000ff );
}

void copyColor(color *in, color *out)
{
    out->r = in->r;
    out->g = in->g;
    out->b = in->b;
    out->hex = colorToHex(*in);
}

void genSpectrum(color colors[], int *numColors)
{
    *numColors = 256 * 6;
    color c;
    c.r = 255;
    c.g = 0;
    c.b = 0;
    copyColor(&c, &colors[0]);
    for (int i=1; i<*numColors; i++)
    {
        if      ( c.r == 255 && c.g <  255 && c.b == 0 )               c.g += 1;  // Red to yellow
        else if ( c.r <= 255 && c.g == 255 && c.b == 0   && c.r != 0 ) c.r -= 1;  // Yellow to green
        else if ( c.r == 0   && c.g == 255 && c.b <  255 )             c.b += 1;  // Green to cyan
        else if ( c.r == 0   && c.g <= 255 && c.b == 255 && c.g != 0 ) c.g -= 1;  // Cyan to blue
        else if ( c.r <  255 && c.g == 0   && c.b == 255 )             c.r += 1;  // Blue to purple
        else if ( c.r == 255 && c.g == 0   && c.b <= 255 && c.b != 0 ) c.b -= 1;  // Purple to red
        copyColor(&c, &colors[i]);
    }
}

void genGradient(color c[], int *numColors, const int start, const int end)
{
    *numColors = 256;
    color cs = hexToColor(start);
    color ce = hexToColor(end);
    float rd = 1.0 * (ce.r - cs.r) / *numColors;
    float gd = 1.0 * (ce.g - cs.g) / *numColors;
    float bd = 1.0 * (ce.b - cs.b) / *numColors;
    for (int i=0; i<*numColors; i++)
    {
        c[i].r = cs.r + (rd * i);
        c[i].g = cs.g + (gd * i);
        c[i].b = cs.b + (bd * i);
    }
}

void genRandom(color c[], int *numColors)
{
    *numColors = 2048;
    srand(time(NULL));
    for (int i=0; i<*numColors; i++)
    {
        c[i].r = rand() % 255;
        c[i].g = rand() % 255;
        c[i].b = rand() % 255;
    }
}

void scaleColor(color colorIn[], color colorOut[], int numColors, int depth, float power)
{
    for (int z = 0; z < depth ; z++)
    {
        double calc = pow((z/(depth-1.0)), power) * numColors;
        colorOut[z].r = colorIn[(int)calc].r;
        colorOut[z].g = colorIn[(int)calc].g;
        colorOut[z].b = colorIn[(int)calc].b;
        colorOut[z].hex = colorToHex(colorOut[z]);
    }
}

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

int output_gd_png(void *_array, int width, int height, int depth, const char *filename, color colors[], int numColors)
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
                color=colors[(*array)[y][x]].hex; //return_color(colors, numColors, depth, (*array)[y][x]);
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

void coord_zoom(coordinates *coord, const double zoom)
{
    double old_xR = coord->xR;
    coord->xR *= pow(.9, zoom);
    coord->x += ( old_xR - coord->xR ) / 2;
    coord->xS = coord->xR/coord->width;

    double old_yR = coord->yR;
    coord->yR *= pow(.9, zoom);
    coord->y -= ( old_yR - coord->yR ) / 2;
    coord->yS = coord->yR/coord->height;
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
    coordinates coord;                   // Coodinates Structure
    int    depth  = 100;                 // Default Depth level of Mandelbrot Calculation
    color  colorsIn[2048];               // Colors array
    int    numColors = 0;                // Number of colors to use in color array
    float  colorPower = .3;              // Power exponent to use for color selection
    int    colorStart = 0xFF0000, colorEnd = 0xFFFF00; // Default color gradient settings
    char   *filename = "mandelbrot.png"; // Default PNG output name
    int    i;                            // Misc variables

    // Set default coordinates before reading in args
    coord.x = -2;      // Default Start Coordinates
    coord.y = 2;
    coord.xR = 4;      // Default Range Coordinates
    coord.yR = 4;
    coord.width = -1;  // Invalid pixel size, to be set later
    coord.height = -1;

    // Generate default gradient first
    genGradient(colorsIn, &numColors, colorStart, colorEnd);

    // Parse Arguments
    for (i=1; i<argc; i++)
     {
        if (strcmp(argv[i], "--width") == 0)
        {
            coord.width = arg_check_int(argc, argv, i, 1, 10);
            i++;
        }
        else if (strcmp(argv[i], "--height") == 0)
        {
            coord.height = arg_check_int(argc, argv, i, 1, 10);
            i++;
        }
        else if (strcmp(argv[i], "--depth") == 0)
        {
            depth = arg_check_int(argc, argv, i, 1, 10);
            i++;
        }
        else if (strcmp(argv[i], "--coords") == 0)
        {
            coord.x  = arg_check_float(argc, argv, i, 1);
            coord.y  = arg_check_float(argc, argv, i, 2);
            coord.xR = arg_check_float(argc, argv, i, 3);
            coord.yR = arg_check_float(argc, argv, i, 4);
            i+=4;
        }
        else if (strcmp(argv[i], "--spectrum") == 0)
        {
            genSpectrum(colorsIn, &numColors);
	    colorPower = .7;
        }
        else if (strcmp(argv[i], "--random") == 0)
        {
            genRandom(colorsIn, &numColors);
	    colorPower = 1;
        }
        else if (strcmp(argv[i], "--gradient") == 0)
        {
            colorStart = arg_check_int(argc, argv, i, 1, 16);
            colorEnd = arg_check_int(argc, argv, i, 2, 16);
            genGradient(colorsIn, &numColors, colorStart, colorEnd);
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

    // Make proportional image if only one dimension is specified
    // Set to default width and height if not specified
    if      (coord.height < 0 && coord.width > 0) coord.height = coord.width  / coord.xR * coord.yR;
    else if (coord.height > 0 && coord.width < 0) coord.width  = coord.height / coord.yR * coord.xR;
    else if (coord.height < 0 && coord.width < 0) coord.width  = coord.height = 1024;

    coord.xS = coord.xR/coord.width;  // X Step Value
    coord.yS = coord.yR/coord.height; // Y Step Value
    int    (*array)[coord.height][coord.width] = malloc(coord.height*coord.width*sizeof(int)); // Array to store values

    // Create final array of colors to use that is scaled to the depth that is selected
    color colors[2048];
    scaleColor(colorsIn, colors, numColors, depth, colorPower);

    //printf("%f %f %f %f %d %d %f %f\n", xStart, yStart, xRange, yRange, width, height, xStep, yStep);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* Main_Window;
    SDL_Renderer* Main_Renderer;
    Main_Window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, coord.width, coord.height, 0);
    Main_Renderer = SDL_CreateRenderer(Main_Window, -1, SDL_RENDERER_ACCELERATED);

    //Main Loop
    while (1)
    {
        // Render Loop
        for (int y=0; y<coord.height; y++)
        {
            for (int x=0; x<coord.width; x++)
            {
                double xValue = coord.x + (x * coord.xS);
                double yValue = coord.y - (y * coord.yS);
                int result = mandel(xValue, yValue, depth);
                (*array)[y][x] = result;
                if (result == -1)
                    SDL_SetRenderDrawColor(Main_Renderer, 0, 0, 0, 255);
                else
                    SDL_SetRenderDrawColor(Main_Renderer, colors[result].r, colors[result].g, colors[result].b, 255);
                SDL_RenderDrawPoint(Main_Renderer, x, y);

            }
            //printf("\r%i%% Complete", ((y+1)*100)/height);
            //fflush(stdout);
        }
        SDL_RenderPresent(Main_Renderer);

        SDL_Event e;
        SDL_WaitEvent(&e);
        if (e.type == SDL_MOUSEWHEEL)
        {
            if (e.wheel.y > 0)
                coord_zoom(&coord, 1);
            else
                coord_zoom(&coord, -1);
        }
        if (e.type == SDL_QUIT)
        {
            SDL_Log("Program quit after %i ticks", e.quit.timestamp);
            break;
        }
    }

    // Output
    //printf("\nWriting to file.\n");
    //output_gd_png(*array, width, height, depth, filename, colors, numColors);

    // Exit
    free(*array);
    return 0;
}
