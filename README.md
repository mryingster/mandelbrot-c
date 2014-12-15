Mandelbrot
==========

Description
-----------
Mandelbrot is a mandelbrot viewer/generator written in C99 using libsdl. It can also output an image to a PNG formatted file using libpng.

Dependencies
------------
  * libpng  - A library used for writing PNG files
  * gd      - A graphics library
  * libsdl2 - Simple DirectMedia Layer used for windowed environment

Compiling
---------
Mandelbrot can be compiled directly in emacs using the `M^x compile` command, otherise, the appropriate flags must be used for the libraries used. 

Usage
-----
When run, Mandelbrot will present the user with a window within which the user can navigate by click and dragging to pan, and scrolling for zoom.
There are command line options (use -h for a list) that allow for the user to set configurable options, such as gradient colors, position, and depth.