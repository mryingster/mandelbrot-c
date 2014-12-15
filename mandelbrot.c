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
#include <stdbool.h>
#include <sys/time.h>
#include <err.h>

uint64_t utime()
{
    struct timeval tp;
    if (gettimeofday(&tp, NULL) != 0)
        err(EXIT_FAILURE, "gettimeofday");
    return tp.tv_sec * 1000000ULL + tp.tv_usec;
}

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

typedef struct mouse mouse;
struct mouse{
    int mouseX;
    int mouseY;
    double coordX;
    double coordY;
    bool down;
};

typedef struct depth depth;
struct depth{
    int d;
    bool automatic;
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
           "    -nw             No Window Mode. Saves directly to file without a gui."
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
        double calc = pow((z/(depth*1.0)), power) * numColors;
        colorOut[z].r = colorIn[(int)calc].r;
        colorOut[z].g = colorIn[(int)calc].g;
        colorOut[z].b = colorIn[(int)calc].b;
        colorOut[z].hex = colorToHex(colorOut[z]);
        //printf("%d, %F, %d, %d, %02X%02X%02X\n", z, calc, numColors, (int)calc, colorOut[z].r, colorOut[z].g, colorOut[z].b);
    }
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

void generate_png(coordinates coord, int depth, char *filename, color colors[], int numColors)
{
    int (*array)[coord.height][coord.width] = malloc(coord.height*coord.width*sizeof(int));

    for (int y=0; y<coord.height; y++)
    {
        for (int x=0; x<coord.width; x++)
        {
            double xValue = coord.x + (x * coord.xS);
            double yValue = coord.y - (y * coord.yS);
            (*array)[y][x] = mandel(xValue, yValue, depth);
        }
        printf("\r%i%% Complete", ((y+1)*100)/coord.height);
        fflush(stdout);
    }
    printf("\nWriting to file, %s.\n", filename);
    output_gd_png(*array, coord.width, coord.height, depth, filename, colors, numColors);

    free(*array);
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

void print_coords(const coordinates *coord, const depth *depth)
{
    printf("Coordinates: (%F, %F); Range: %F, %F; Pitch: %F; Depth: %d\n",
           coord->x, coord->y, coord->xR, coord->yR, coord->xS, depth->d);
}

void adjust_depth(const coordinates *coord, depth *depth)
{
    depth->d = 60.2 * pow(coord->xR, -.163);
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
    depth depth;
    color  colorsIn[2048];               // Colors array
    int    numColors = 0;                // Number of colors to use in color array
    float  colorPower = .3;              // Power exponent to use for color selection
    int    colorStart = 0xFF0000, colorEnd = 0xFFFF00; // Default color gradient settings
    char   *filename = "mandelbrot.png"; // Default PNG output name
    int    i;                            // Misc variables
    bool   noWindow = false;             // Default flag for no-window mode

    // Set default coordinates before reading in args
    coord.x = -2;          // Default Start Coordinates
    coord.y = 2;
    coord.xR = 4;          // Default Range Coordinates
    coord.yR = 4;
    coord.width = -1;      // Invalid pixel size, to be set later
    coord.height = -1;
    depth.d = 100;         // Default Depth level of Mandelbrot Calculation
    depth.automatic = true;

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
            depth.automatic = false;
            depth.d = arg_check_int(argc, argv, i, 1, 10);
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
        else if (strcmp(argv[i], "-nw") == 0 )
        {
            noWindow = true;
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

    // Create final array of colors to use that is scaled to the depth that is selected
    color colors[2048];
    scaleColor(colorsIn, colors, numColors, depth.d, colorPower);

    // If no window mode, just output file and exit
    if (noWindow)
    {
        generate_png(coord, depth.d, filename, colors, numColors);
        return 0;
    }

    // Set up SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* Main_Window;
    SDL_Renderer* Main_Renderer;
    Main_Window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, coord.width, coord.height, SDL_WINDOW_RESIZABLE);
    Main_Renderer = SDL_CreateRenderer(Main_Window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(Main_Renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, coord.width, coord.height);
    int pitch;
    void *pixels;
    if (SDL_LockTexture(texture, NULL, &pixels, &pitch) != 0)
        errx(EXIT_FAILURE, "SDL_LockTexture: %s", SDL_GetError());
    //uint32_t (*pixel)[coord.height][pitch/sizeof(uint32_t)] = pixels;

    // Set up struct for tracking mouse movement
    mouse mouseTracker;
    mouseTracker.down = false;

    //Main Loop
    int maxRender = 2;
    int needsRender = maxRender;
    while (1)
    {
        // Render Loop
        if (needsRender > 0)
        {
            uint32_t (*pixel)[coord.height][pitch/sizeof(uint32_t)] = pixels;

            //unsigned long long timer = utime(); //DEBUG - Start Timer
            float pixelSize = pow(2, --needsRender);
            //printf("%d %d\n", needsRender, pixelSize);

            coordinates scaledCoord;
            scaledCoord.width = coord.width / pixelSize;
            scaledCoord.height = coord.height / pixelSize;
            scaledCoord.x  = coord.x;
            scaledCoord.xR = coord.xR;
            scaledCoord.xS = scaledCoord.xR/scaledCoord.width;
            scaledCoord.y  = coord.y;
            scaledCoord.yR = coord.yR;
            scaledCoord.yS = scaledCoord.yR/scaledCoord.height;     // y step value
            SDL_Rect srcrect = {0, 0, scaledCoord.width, scaledCoord.height};

            for (int y=0; y<scaledCoord.height; y++)
                for (int x=0; x<scaledCoord.width; x++)
                {
                    double xValue = scaledCoord.x + (x * scaledCoord.xS);
                    double yValue = scaledCoord.y - (y * scaledCoord.yS);
                    int result = mandel(xValue, yValue, depth.d);

                    int finalColor = 0;
                    if (result != -1)
                        finalColor = colors[result].hex << 8;

                    (*pixel)[y][x] = finalColor;
                }

            SDL_UnlockTexture(texture);
            SDL_RenderCopy(Main_Renderer, texture, &srcrect, NULL);

            //printf("%llu - Finish Render\n", utime()-timer); //DEBUG - End Timer

            SDL_RenderPresent(Main_Renderer);
        }

        SDL_Event e;
        //SDL_WaitEvent(&e);
        if (SDL_WaitEventTimeout(&e, 10) == 0) continue;
        if (e.type == SDL_MOUSEWHEEL)
        {
            if (e.wheel.y > 0)
                coord_zoom(&coord, 1);
            else
                coord_zoom(&coord, -1);
            if (depth.automatic == true)
            {
                adjust_depth(&coord, &depth);
                scaleColor(colorsIn, colors, numColors, depth.d, colorPower);
            }
            needsRender = maxRender;
        }

        if (e.type == SDL_WINDOWEVENT)
            if (e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                // Adjust view to new size
                coord.width = e.window.data1;
                coord.height = e.window.data2;
                coord.xR = coord.width * coord.xS;
                coord.yR = coord.height * coord.yS;

                // Destroy old texture
                SDL_DestroyTexture(texture);

                // Make New Texture
                texture = SDL_CreateTexture(Main_Renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, coord.width, coord.height);

                // Lock with new texture
                if (SDL_LockTexture(texture, NULL, &pixels, &pitch) != 0)
                    errx(EXIT_FAILURE, "SDL_LockTexture: %s", SDL_GetError());

                // Rerender
                needsRender = maxRender;
            }

        if (e.type == SDL_MOUSEBUTTONDOWN)
            if (e.button.state == SDL_PRESSED)
            {
                mouseTracker.mouseX = e.motion.x;
                mouseTracker.mouseY = e.motion.y;
                mouseTracker.coordX = coord.x;
                mouseTracker.coordY = coord.y;
                mouseTracker.down = true;
            }

        if (e.type == SDL_MOUSEBUTTONUP)
            if (e.button.state == SDL_RELEASED)
                mouseTracker.down = false;

        if (e.type == SDL_MOUSEMOTION && mouseTracker.down == true)
        {
            coord.x = mouseTracker.coordX + ((mouseTracker.mouseX - e.motion.x) * coord.xS);
            coord.y = mouseTracker.coordY - ((mouseTracker.mouseY - e.motion.y) * coord.yS);
            needsRender = maxRender;
        }

        if (e.type == SDL_KEYUP)
        {
            if (e.key.keysym.sym == SDLK_p)
                print_coords(&coord, &depth);
            if (e.key.keysym.sym == SDLK_s)
                generate_png(coord, depth.d, filename, colors, numColors);
            if (e.key.keysym.sym == SDLK_EQUALS || e.key.keysym.sym == SDLK_MINUS)
            {
                depth.automatic = false;
                if (e.key.keysym.sym == SDLK_EQUALS)
                    depth.d += 5;
                if (e.key.keysym.sym == SDLK_MINUS)
                    if (depth.d > 5) depth.d -= 5;
                scaleColor(colorsIn, colors, numColors, depth.d, colorPower);
                needsRender = maxRender;
            }
        }

        if (e.type == SDL_QUIT)
        {
            SDL_Log("Program quit after %i ticks", e.quit.timestamp);
            break;
        }
    }

    return 0;
}
