#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gd.h>

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


int hex_color(int r, int g, int b)
{
    return r*0x10000+g*0x100+b;
}

int output_gd_png(void *_array, int width, int height, int depth)
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
                color=hex_color(0, 255/depth*(*array)[y][x], 0);
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

int main()
{
    double xStart=-2, yStart=-2;     // Start Coordinates
    double xRange=4,  yRange=4;      // Range Coordinates
    int    width=512, height=512;    // Pixel size of output
    double xStep = xRange/width;     // x Step Value
    double yStep = yRange/height;    // y Step Value
    double xValue, yValue;           // X Y Values at Pixel Locations
    int    depth = 50;               // Level of depth for Mandelbrot Calculation
    int    x, y;                     // Other variables
    int    array[height][width];     // Array to store values

    // Main loop
    for (y=0; y<height; y++)
    {
        for (x=0; x<width; x++)
        {
            xValue = xStart + (x * xStep);
            yValue = yStart + (y * yStep);
            array[y][x] = mandel(xValue, yValue, depth);
        }
        printf("\r%i%% Complete", ((y+1)*100)/height);
        fflush(stdout);
    }

    // Output
    printf("\nWriting to file.\n");
    output_gd_png(*array, width, height, depth);

    // Exit
    free(*array);
    return 0;
}
