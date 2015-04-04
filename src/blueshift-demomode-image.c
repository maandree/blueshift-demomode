/**
 * blueshift-demomode — Blueshift-effect demonstration tools
 * Copyright Ⓒ 2014, 2015  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



/**
 * Value for `tupletype` when the image is black and white
 * The bytes are: whitenames (note that it is not blackness as with P1 and P4)
 * (note that it is bytes and not bits as with P4)
 */
#define BLACKANDWHITE  0

/**
 * Value for `tupletype` when the image is grey
 * The words are: whiteness
 */
#define GRAYSCALE  1

/**
 * Value for `tupletype` when the image is coloured
 * The words are: redness, greenness, blueness
 */
#define RGB  2

/**
 * Value for `tupletype` when the image is black and white with an alpha channel
 * The bytes are: blackness (note that it is not blackness as with P1 and P4), transparency
 */
#define BLACKANDWHITE_ALPHA  3

/**
 * Value for `tupletype` when the image is grey with an alpha channel
 * The words are: whiteness, transparency
 */
#define GRAYSCALE_ALPHA  4

/**
 * Value for `tupletype` when the image is coloured with an alpha channel
 * The words are: redness, greenness, blueness, transparency
 */
#define RGB_ALPHA  5



/**
 * The width of image, in pixels
 */
static size_t width = 0;
static int have_width = 0;

/**
 * The height of the image, in pixels
 */
static size_t height = 0;
static int have_height = 0;

/**
 * The number of channals, 1 for black and white or greyscale, 3 for colour, plus 1 for alpha
 */
static int depth = 0;
static int have_depth = 0;

/**
 * The maximum value a channel can have, the white point threshold
 */
static uint32_t maxval = 0;
static int have_maxval = 0;

/**
 * The channel configuration
 */
static int tupltype = 0;
static int have_tupltype = 0;


static size_t red_n, green_n, blue_n;
static uint32_t* max = NULL;
static uint32_t* red = NULL;
static uint32_t* green = NULL;
static uint32_t* blue = NULL;

static char* execname;



static void strip(char* buffer)
{
  size_t offset = 0;
  size_t end = strlen(buffer);
  while (buffer[offset] && ((buffer[offset] == ' ') || (buffer[offset] == '\t')))
    offset++;
  while (end && ((buffer[end - 1] == ' ') || (buffer[end - 1] == '\t')))
    end--;
  buffer[end] = '\0';
  memmove(buffer, buffer + offset, end + 1 - offset);
}


static long long conv(char* buffer)
{
  strip(buffer);
  return atoll(buffer);
}


static uint32_t* int_split(char* buffer, size_t* out_n)
{
  size_t n = 1;
  char* buf = buffer;
  uint32_t* rc = NULL;
  uint32_t v = 0;
  
  while (*buf)
    if (*buf++ == ' ')
      n++;
  
  if (out_n)
    *out_n = n;
  rc = malloc(n * sizeof(uint32_t));
  if (rc == NULL)
    return NULL;
  
  for (n = 0; *buf; buf++)
    if (*buf == ' ')
      rc[n++] = v, v = 0;
    else
      v = (v * 10) + (*buf & 15);
  rc[n] = v;
  
  return rc;
}


static inline void conv1(unsigned char* in, unsigned char* out, uint32_t* lut, size_t lutsize, uint32_t lutmax)
{
  size_t n = ((uint32_t)*in * lutsize + maxval / 2) / maxval;
  if (n > lutsize)
    n = lutsize;
  *out = (unsigned char)(lut[n] * maxval / lutmax);
}


static inline void conv2(unsigned char* in, unsigned char* out, uint32_t* lut, size_t lutsize, uint32_t lutmax)
{
  size_t n = (((uint32_t)(in[0])) << 8) | ((uint32_t)(in[1]));
  n = (n * lutsize + maxval / 2) / maxval;
  if (n > lutsize)
    n = lutsize;
  n = lut[n] * maxval / lutmax;
  out[0] = (n >> 8) & 255;
  out[1] = (n >> 0) & 255;
}


#define FILTER(FOR, SIZE, RGB, WPTR)								\
  for FOR											\
    {												\
      conv##SIZE(data + ptr + RGB * SIZE * 0, data + WPTR + SIZE * 0, red,     red_n, max[0]);	\
      conv##SIZE(data + ptr + RGB * SIZE * 1, data + WPTR + SIZE * 1, green, green_n, max[1]);	\
      conv##SIZE(data + ptr + RGB * SIZE * 2, data + WPTR + SIZE * 2, blue,   blue_n, max[2]);	\
    }


static int filter(size_t size, int rgb, int alpha, int dual)
{
  char* data = malloc(size * (rgb ? 1 : 3));
  size_t ptr = 0;
  ssize_t got;
  if (data == NULL)
    goto fail;
  
  while (size)
    {
      got = read(0, data + ptr, size);
      if (got < 0)
	goto fail;
      ptr += (size_t)got;
      size -= (size_t)got;
    }
  
  size = ptr;
  
  if      (rgb && alpha && dual)  FILTER((ptr = 0; ptr < size; ptr += 8), 2, 1, ptr)
  else if (rgb && alpha)          FILTER((ptr = 0; ptr < size; ptr += 4), 1, 1, ptr)
  else if (rgb && dual)           FILTER((ptr = 0; ptr < size; ptr += 6), 2, 1, ptr)
  else if (rgb)                   FILTER((ptr = 0; ptr < size; ptr += 3), 1, 1, ptr)
  else if (alpha && dual)         FILTER((; ptr; ptr -= 4, size -= 8),    2, 0, size - 8)
  else if (alpha)                 FILTER((; ptr; ptr -= 2, size -= 4),    1, 0, size - 4)
  else if (dual)                  FILTER((; ptr; ptr -= 2, size -= 6),    2, 0, size - 6)
  else                            FILTER((; ptr; ptr -= 1, size -= 3),    1, 0, size - 3)
  
  return 0;
 fail:
  perror(execname);
  free(data);
  return -1;
}


int main(int argc, char** argv)
{
  int ok = 0;
  int stage = 0;
  char buf[1024];
  size_t ptr = 0, size;
  int alpha;
  
  execname = *argv;
  
  
  for (;;)
    {
      int c = getchar();
      if (c < 0)
	goto bad;
      if (c == '\n')
	{
	  buf[ptr] = '\0';
	  ptr = 0;
	  strip(buf);
	  if (!strcmp(buf, "P7"))
	    {
	      if (stage++)
		break;
	    }
	  else if (!strcmp(buf, "ENDHDR"))
	    {
	      ok = 1;
	      break;
	    }
	  else if (strstr(buf, "WIDTH") == buf)
	    {
	      if (have_width)  break;
	      have_width = 1, width = (size_t)conv(buf + 5);
	    }
	  else if (strstr(buf, "HEIGHT") == buf)
	    {
	      if (have_height)  break;
	      have_height = 1, height = (size_t)conv(buf + 6);
	    }
	  else if (strstr(buf, "DEPTH") == buf)
	    {
	      if (have_depth)  break;
	      have_depth = 1, depth = (int)conv(buf + 5);
	    }
	  else if (strstr(buf, "MAXVAL") == buf)
	    {
	      if (have_maxval)  break;
	      have_maxval = 1, maxval = (uint32_t)conv(buf + 6);
	    }
	  else if (strstr(buf, "TUPLTYPE") == buf)
	    {
	      if (have_tupltype)  break;
	      strip(buf + 8);
	      have_tupltype = 1;
	      if      (!strcmp(buf + 8, "BLACKANDWHITE"))        tupltype = BLACKANDWHITE;
	      else if (!strcmp(buf + 8, "GRAYSCALE"))            tupltype = GRAYSCALE;
	      else if (!strcmp(buf + 8, "RGB"))                  tupltype = RGB;
	      else if (!strcmp(buf + 8, "BLACKANDWHITE_ALPHA"))  tupltype = BLACKANDWHITE_ALPHA;
	      else if (!strcmp(buf + 8, "GRAYSCALE_ALPHA"))      tupltype = GRAYSCALE_ALPHA;
	      else if (!strcmp(buf + 8, "RGB_ALPHA"))            tupltype = RGB_ALPHA;
	      else
		goto bad;
	    }
	}
      else
	if (ptr + 1 == sizeof(buf) / sizeof(*buf))
	  goto bad;
	else
	  buf[ptr++] = (char)c;
    }
  
  if (!ok || !width || !height || !depth || !maxval || !tupltype)
    ok = 0;
  else if ((width < 1) || (height < 0))      ok = 0;
  else if (tupltype == BLACKANDWHITE)        ok = (depth == 1) && (1 <= maxval) && (maxval <= 1);
  else if (tupltype == GRAYSCALE)            ok = (depth == 1) && (1 <= maxval) && (maxval <= 65535);
  else if (tupltype == RGB)                  ok = (depth == 3) && (1 <= maxval) && (maxval <= 65535);
  else if (tupltype == BLACKANDWHITE_ALPHA)  ok = (depth == 2) && (1 <= maxval) && (maxval <= 1);
  else if (tupltype == GRAYSCALE_ALPHA)      ok = (depth == 2) && (1 <= maxval) && (maxval <= 65535);
  else if (tupltype == RGB_ALPHA)            ok = (depth == 4) && (1 <= maxval) && (maxval <= 65535);
  if (!ok)
    goto bad;
  
  if ((max   = int_split(argv[1],     NULL)) == NULL)  goto fail;
  if ((red   = int_split(argv[2],   &red_n)) == NULL)  goto fail;
  if ((green = int_split(argv[3], &green_n)) == NULL)  goto fail;
  if ((blue  = int_split(argv[4],  &blue_n)) == NULL)  goto fail;
  alpha = tupltype >= BLACKANDWHITE_ALPHA;
  
  
  printf("P7\n");
  printf("WIDTH %zu\n", width);
  printf("HEIGHT %zu\n", height);
  printf("DEPTH %i\n", alpha ? 4 : 3);
  printf("MAXVAL %zu\n", (size_t)maxval);
  printf("TUPLTYPE %s\n", alpha ? "RGB_ALPHA" : "RGB");
  printf("ENDHDR\n");
  
  
  red_n--;
  green_n--;
  blue_n--;
  size = width * height * depth * (maxval > 255 ? 2 : 1);
  if (filter(size, tupltype == (alpha ? RGB_ALPHA : RGB), alpha, maxval > 255))
    goto fail;
  
  
  free(max);
  free(red);
  free(green);
  free(blue);
  return 0;
 bad:
  fprintf(stderr, "%s: %s\n", execname, "Failed to load image");
  return 1;
 fail:
  perror(execname);
  free(max);
  free(red);
  free(green);
  free(blue);
  return 1;
}

