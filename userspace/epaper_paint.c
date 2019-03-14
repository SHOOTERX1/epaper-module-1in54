/**
 *  Copyright (C) Waveshare     September 9 2017
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
 * SHOOTERX1 <yorha.a2@foxmail.com>
 */
#include <stdlib.h>
#include <errno.h>

#include "epaper_paint.h"
#include "fonts.h"

/* Functions to modify properties */
struct epd_paint *epdpaint_init(int width, int height, int rotate)
{
    struct epd_paint *p;
    p = (struct epd_paint *) malloc(sizeof(struct epd_paint));
    if (!p)
        return NULL;
    p->width = width % 8 ? width + 8 - (width % 8) : width;
    p->height = height;
    p->rotate = rotate;
    p->frame_buffer = (unsigned char *) malloc(p->width * p->height);
    if(!p->frame_buffer)
        return NULL;
    return p;
}

struct epd_paint *epdpaint_init_with_exist_image_array(
        int width, int height, int rotate,
        const unsigned char *arr, int arr_size)
{
    struct epd_paint *p;
    if(arr_size < (width * height / 8))
        return NULL;
    p = epdpaint_init(width, height, rotate);
    /* copy data */
    for(int i=0; i < arr_size; i++)
        p->frame_buffer[i] = arr[i];
    return p;
}

void epdpaint_release(struct epd_paint *paint)
{
    free(paint->frame_buffer);
    free(paint);
}

void epdpaint_clear(struct epd_paint *paint ,int colored)
{
    int x, y;
    for(x = 0; x < paint->width; x++)
        for (y = 0; y < paint->height; y++)
            epdpaint_draw_absolute_pixel(paint, x, y, colored);
}

/* Functions to operate the flame buffer */
void epdpaint_draw_absolute_pixel(struct epd_paint *paint,
                    int x, int  y, int colored)
{
    if (x < 0 || x >= paint->width || y < 0 || y >= paint->height)
        return;
    if (IF_INVERT_COLOR) {
        if (colored)
            paint->frame_buffer[(x + y * paint->width) / 8] |= 0x80 >> (x % 8);
        else
            paint->frame_buffer[(x + y * paint->width) / 8] &= ~(0x80 >> (x % 8));
    } else {
        if (colored)
            paint->frame_buffer[(x + y * paint->width) / 8] &= ~(0x80 >> (x % 8));
        else
            paint->frame_buffer[((x + y * paint->width) / 8)] |= 0x80 >> (x % 8);
    }
}

void epdpaint_draw_pixel(struct epd_paint *paint,
                    int x, int y, int colored)
{
    int point_temp;
    if (paint->rotate == ROTATE_0) {
        if (x < 0 || x >= paint->width || y < 0 || y >= paint->height)
            return;
    }  else if (paint->rotate == ROTATE_90) {
        if (x < 0 || x >= paint-> height || y < 0 || y >= paint-> width)
            return;
        point_temp = x;
        x = paint->width - y;
        y = point_temp;
    } else if (paint->rotate == ROTATE_180) {
        if(x < 0 || x >= paint->width || y < 0 || y >= paint->height)
            return;
        x = paint->width - x;
        y = paint->width - y;
    } else if (paint->rotate == ROTATE_270) {
        if (x < 0 || x >= paint-> height || y < 0 || y >= paint-> width)
            return;
        point_temp = x;
        x = y;
        y = paint->height - point_temp;
    }
    epdpaint_draw_absolute_pixel(paint, x, y, colored);
}

void epdpaint_draw_char_at(struct epd_paint *paint,
                    int x, int y, char ascii_char,
                    sFONT *font, int colored)
{
    int i, j;
    unsigned int char_offset = (ascii_char - ' ') *
            font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &font->table[char_offset];

    for (j = 0; j < font->Height; j++) {
        for(i = 0; i < font->Width; i++) {
            if (*ptr & (0x80 >> (i %8)))
                epdpaint_draw_pixel(paint, x + i, y + j, colored);
            if (i % 8 == 7)
                ptr++; 
        }
        if (font->Width % 8 != 0)
            ptr++;
    }
}

size_t epdpaint_draw_string_at(struct epd_paint *paint,
                    int x, int y, const char *text,
                    sFONT *font, int colored)
{
    const char *p_text = text;
    size_t counter = 0;
    int refcolum = x;

    while (*p_text != 0) {
        epdpaint_draw_char_at(paint, refcolum, y, *p_text, font, colored);
        refcolum += font->Width;
        p_text++;
        counter++;
    }
    return counter;
}

void epdpaint_draw_line(struct epd_paint *paint,
                    int x0, int y0, int x1, int y1,
                    int colored)
{
    /* Bresenham algorithm */
    int dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while((x0 != x1) && (y0 != y1)) {
        epdpaint_draw_pixel(paint, x0, y0 , colored);
        if (2 * err >= dy) {     
            err += dy;
            x0 += sx;
        }
        if (2 * err <= dx) {
            err += dx; 
            y0 += sy;
        }
    }
}

void epdpaint_draw_horizontal_line(struct epd_paint *paint,
                    int x, int y, int line_width, int colored)
{
    for (int i = x; i < x + line_width; i++)
        epdpaint_draw_pixel(paint, i, y, colored);
}

void epdpaint_draw_vertical_line(struct epd_paint *paint,
                    int x, int y, int line_height, int colored)
{
    for (int i = y; i < y + line_height; i++)
        epdpaint_draw_pixel(paint, x, i, colored);
}

void epdpaint_draw_rectangle(struct epd_paint *paint,
                    int x0, int y0, int x1, int y1, int colored)
{
    int min_x, min_y, max_x, max_y;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y0 : y1;

    epdpaint_draw_horizontal_line(paint, min_x, min_y, max_x - min_x + 1, colored);
    epdpaint_draw_horizontal_line(paint, min_x, max_y, max_x - min_x + 1, colored);
    epdpaint_draw_vertical_line(paint, min_x, min_y, max_y - min_y + 1, colored);
    epdpaint_draw_vertical_line(paint, max_x, min_y, max_y - min_y + 1, colored);
}

void epdpaint_draw_filled_rectangle(struct epd_paint *paint,
                    int x0, int y0, int x1, int y1, int colored)
{
    int min_x, min_y, max_x, max_y;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y0 : y1;

    for (int i = min_x; i <= max_x; i++)
        epdpaint_draw_vertical_line(paint, i, min_y, max_y - min_y + 1, colored);
}

void epdpaint_draw_circle(struct epd_paint *paint,
                    int x, int y, int radius, int colored)
{
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        epdpaint_draw_pixel(paint, x - x_pos, y + y_pos, colored);
        epdpaint_draw_pixel(paint, x + x_pos, y + y_pos, colored);
        epdpaint_draw_pixel(paint, x + x_pos, y - y_pos, colored);
        epdpaint_draw_pixel(paint, x - x_pos, y - y_pos, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
              e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

void epdpaint_draw_filled_circle(struct epd_paint *paint,
                    int x, int y, int radius, int colored)
{
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        epdpaint_draw_pixel(paint, x - x_pos, y + y_pos, colored);
        epdpaint_draw_pixel(paint, x + x_pos, y + y_pos, colored);
        epdpaint_draw_pixel(paint, x + x_pos, y - y_pos, colored);
        epdpaint_draw_pixel(paint, x - x_pos, y - y_pos, colored);
        epdpaint_draw_horizontal_line(paint, x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored);
        epdpaint_draw_horizontal_line(paint, x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if(e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while(x_pos <= 0);
}

int epdpaint_get_width(struct epd_paint *paint)
{
    return paint->width;
}

void epdpaint_set_width(struct epd_paint *paint ,int width)
{
    /* 1 byte = 8 pixels, so the width should be the multiple of 8 */
    paint->width = width % 8 ? width + 8 -(width % 8) : width;
}
int epdpaint_get_height(struct epd_paint *paint)
{
    return paint->height;
}
void epdpaint_set_height(struct epd_paint *paint, int height)
{
    paint->height = height;
}
int epdpaint_get_rotate(struct epd_paint *paint)
{
    return paint->rotate;
}
void epdpaint_set_rotate(struct epd_paint *paint, int rotate)
{
    paint->rotate = rotate;
}
unsigned char *epdpaint_get_image(struct epd_paint *paint)
{
    return paint->frame_buffer;
}