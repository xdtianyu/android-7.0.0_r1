// Copyright 2002-2010 Guillaume Cottenceau.
// This software may be freely redistributed under the terms of
// the X11 license.
//
// Function write_png_file taken slightly modified from
// http://zarb.org/~gc/html/libpng.html

#include <png.h>
#include <stdarg.h>
#include <stdio.h>

#include <base/files/file_util.h>
#include <base/memory/scoped_ptr.h>
#include <gflags/gflags.h>

#include "png_helper.h"

void abort_(const char * s, ...) {
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}

void write_png_file(const char* file_name, char* pixels, int width, int height)
{
  int         x, y;
  png_bytep  *row_pointers;
  png_structp png_ptr;
  png_infop   info_ptr;
  png_byte    bit_depth = 8;  // 8 bits per channel RGBA
  png_byte    color_type = 6; // RGBA

  row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
  char *p = pixels;
  for (y=height-1; y>=0; y--) {
    row_pointers[y] = (png_byte*) malloc(4*width);
    for (x=0; x<width; x++) {
      png_byte* pixel = &(row_pointers[y][x*4]);
      pixel[0] = *p; p++; // R
      pixel[1] = *p; p++; // G
      pixel[2] = *p; p++; // B
      pixel[3] = *p; p++; // A
    }
  }

  /* create file */
  FILE *fp = fopen(file_name, "wb");
  if (!fp)
    abort_("[write_png_file] File %s could not be opened for writing",
           file_name);
  /* initialize stuff */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    abort_("[write_png_file] png_create_write_struct failed");
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    abort_("[write_png_file] png_create_info_struct failed");
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[write_png_file] Error during init_io");
  png_init_io(png_ptr, fp);

  /* write header */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[write_png_file] Error during writing header");
  png_set_IHDR(png_ptr, info_ptr, width, height,
               bit_depth, color_type, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);

  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[write_png_file] Error during writing bytes");
  png_write_image(png_ptr, row_pointers);

  /* end write */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[write_png_file] Error during end of write");
  png_write_end(png_ptr, NULL);

  /* cleanup heap allocation */
  for (y=0; y<height; y++)
    free(row_pointers[y]);
  free(row_pointers);

  fclose(fp);

  // Try to flush saved image to disk such that more data survives a hard crash.
  system("/bin/sync");
}
