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
/**
 * ###########################################################
 * 
 * running in userspace!
 * 
 * ###########################################################
*/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "epaper_core.h"
#include "epaper_paint.h"

#define COLORED     0
#define UNCOLORED   1
#define ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof((arr)[0])))

extern const unsigned char epaper_background[5000];

int main(void)
{
    struct epd_s *epd;
    struct epd_paint *paint;
    if ((epd = epd_create_epaper()) == NULL)
        return -ENOMEM;
    epd_init_epaper(epd, lut_full_update);
    if ((paint = epdpaint_init_with_exist_image_array(epd->width, epd->height,
                ROTATE_0, epaper_background,
                ARRAY_SIZE(epaper_background))) == NULL)
        return -ENOMEM;
    

    epd_set_frame_memory(epd, epdpaint_get_image(paint), 0, 0,
                    epdpaint_get_width(paint), epdpaint_get_height(paint));
    epd_display_frame(epd);
    epd_delay_ms(2000);

    epd_epaper_sleep(epd);
    epdpaint_release(paint);
    epd_release_epaper(epd);

    return 0;
}