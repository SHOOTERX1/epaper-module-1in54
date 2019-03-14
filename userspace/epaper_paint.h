/**
 *  Copyright (C) Waveshare     July 28 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * Rewritten to apply to C language
 * yorha.a2 <yorha.a2@foxmail.com>
 */

#if !defined(EPAPER_PAINT_H)
#define EPAPER_PAINT_H

#include "fonts.h"
#include <stddef.h>

/* Display orientation */
#define ROTATE_0            0
#define ROTATE_90           1
#define ROTATE_180          2
#define ROTATE_270          3

/* Color inverse. 1 or 0 = set or reset a bit if set a colored pixel */
#define IF_INVERT_COLOR     1

/**
 * A structure representing a frame of picture
 */
struct epd_paint {
    int width;
    int height;
    int rotate;
    unsigned char *frame_buffer;
};

/* Functions to modify properties */
struct epd_paint *epdpaint_init(int width, int height, int rotate);
void epdpaint_release(struct epd_paint *paint);
void epdpaint_clear(struct epd_paint *paint ,int colored);
struct epd_paint *epdpaint_init_with_exist_image_array(
        int width, int height, int rotate,
        const unsigned char *arr, int arr_size);
/* Getter and setter */
int epdpaint_get_width(struct epd_paint *paint);
void epdpaint_set_width(struct epd_paint *paint ,int width);
int epdpaint_get_height(struct epd_paint *paint);
void epdpaint_set_height(struct epd_paint *paint ,int height);
int epdpaint_get_rotate(struct epd_paint *paint);
void epdpaint_set_rotate(struct epd_paint *paint ,int rotate);
unsigned char *epdpaint_get_image(struct epd_paint *paint);

/* Functions to operate the flame buffer */
void epdpaint_draw_absolute_pixel(struct epd_paint *paint,
                    int x, int  y, int colored);
void epdpaint_draw_pixel(struct epd_paint *paint,
                    int x, int y, int colored);
void epdpaint_draw_char_at(struct epd_paint *paint,
                    int x, int y, char ascii_char,
                    sFONT *font, int colored);
size_t epdpaint_draw_string_at(struct epd_paint *paint,
                    int x, int y, const char *text,
                    sFONT *font, int colored);
void epdpaint_draw_line(struct epd_paint *paint,
                    int x0, int y0, int x1, int y1,
                    int colored);
void epdpaint_draw_horizontal_line(struct epd_paint *paint,
                    int x, int y, int line_width, int colored);
void epdpaint_draw_vertical_line(struct epd_paint *paint,
                    int x, int y, int line_height, int colored);
void epdpaint_draw_rectangle(struct epd_paint *paint,
                    int x0, int y0, int x1, int y1, int colored);
void epdpaint_draw_filled_rectangle(struct epd_paint *paint,
                    int x0, int y0, int x1, int y1, int colored);
void epdpaint_draw_circle(struct epd_paint *paint,
                    int x, int y, int radius, int colored);
void epdpaint_draw_filled_circle(struct epd_paint *paint,
                    int x, int y, int radius, int colored);

#endif // EPAPER_PAINT_H
