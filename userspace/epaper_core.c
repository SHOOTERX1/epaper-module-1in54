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
 * SHOOTERX1 <yorha.a2@foxmail.com>
 */
/**
 * ================================================================
 * epaepr_core.c        ----  lib file
 * provides functions use in userspace.
 * ================================================================
 */

#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "epaper_core.h"
#include "epaper_cmds.h"


static void epd_set_lut(struct epd_s *e, const unsigned char *l)
{
    epd_send_cmd(e, WRITE_LUT_REGISTER);
    /* the length of look-up table is 30 bytes */
    for (int i = 0; i < 30; i++)
        epd_send_data(e, l[i]);
}

static void
epd_set_memory_area(struct epd_s *e, int x_start, int y_start, int x_end, int y_end)
{
    epd_send_cmd(e, SET_RAM_X_ADDRESS_START_END_POSITION);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    epd_send_data(e, (x_start >> 3) & 0xFF);
    epd_send_data(e, (x_end >> 3) & 0xFF);
    epd_send_cmd(e, SET_RAM_Y_ADDRESS_START_END_POSITION);
    epd_send_data(e, y_start & 0xFF);
    epd_send_data(e, (y_start >> 8) & 0xFF);
    epd_send_data(e, y_end & 0xFF);
    epd_send_data(e, (y_end >> 8) & 0xFF);
}

static void epd_set_memory_pointer(struct epd_s *e, int x, int y)
{
    epd_send_cmd(e, SET_RAM_X_ADDRESS_COUNTER);
    epd_send_data(e, (x >> 3) & 0xFF);
    epd_send_cmd(e, SET_RAM_Y_ADDRESS_COUNTER);
    epd_send_data(e, y & 0xFF);
    epd_send_data(e, (y >> 8) & 0xFF);
    epd_wait_until_idle(e);
}

void epd_delay_ms(unsigned int ms)
{
    usleep(ms * 1000);
}

struct epd_s *epd_create_epaper(void)
{
    struct epd_s *e;
    e = (struct epd_s *) malloc(sizeof(struct epd_s));
    if(!e)
        return NULL;
    e->fd = open(EPAPER_SPI_DEV_PATH, O_WRONLY);
    if (e->fd < 0)
        return NULL;
    e->width = EPD_WIDTH;
    e->height = EPD_HEIGHT;
    return e;
}

void epd_init_epaper(struct epd_s *e, const unsigned char *l)
{
    /* EPD hardware init start */
    epd_reset(e);
    epd_send_cmd(e, DRIVER_OUTPUT_CONTROL);
    epd_send_data(e, (EPD_HEIGHT -1) & 0xFF);
    epd_send_data(e, ((EPD_HEIGHT - 1) >> 8) & 0xFF);
    epd_send_data(e, 0x00);
    epd_send_cmd(e, BOOSTER_SOFT_START_CONTROL);
    epd_send_data(e, 0xD7);
    epd_send_data(e, 0xD6);
    epd_send_data(e, 0x9D);
    epd_send_cmd(e, WRITE_VCOM_REGISTER);
    epd_send_data(e, 0xA8);
    epd_send_cmd(e, SET_DUMMY_LINE_PERIOD);
    epd_send_data(e, 0x08);
    epd_send_cmd(e, DATA_ENTRY_MODE_SETTING);
    epd_send_data(e, 0x03);
    epd_set_lut(e, l);
    /* EPD hardware init end */
}

void epd_release_epaper(struct epd_s *e)
{
    close(e->fd);
    free(e);
}

size_t epd_spi_transfer(struct epd_s *e, const char c)
{
    return write(e->fd, &c, 1);
}

int epd_send_data(struct epd_s *e, const char c)
{
    ioctl(e->fd, EPAPER_DC_PIN_SET_HIGH);
    if (epd_spi_transfer(e, c) < 0)
        exit(-EIO);
    return 0;
}

int epd_send_cmd(struct epd_s *e, const char c)
{
    ioctl(e->fd, EPAPER_DC_PIN_SET_LOW);
    return epd_spi_transfer(e, c);
}

int epd_wait_until_idle(struct epd_s *e)
{
    while (ioctl(e->fd, EPAPER_IS_DEV_BUSY) == EPD_BUSY)
        epd_delay_ms(100);  // 100ms
}

void epd_reset(struct epd_s *e)
{
    ioctl(e->fd, EPAPER_RESET);
}

void epd_set_frame_memory(struct epd_s *e,
        const unsigned char *image_buffer,
        int x, int y, int image_width, int image_height)
{
    int x_end;
    int y_end;

    if (image_buffer == NULL ||
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0)
        return;
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    x &= 0xF8;
    image_width &= 0xF8;
    if (x + image_width >= e->width) {
        x_end = e->width - 1;
    } else {
        x_end = x + image_width - 1;
    }
    if (y + image_height >= e->height) {
        y_end = e->height - 1;
    } else {
        y_end = y + image_height - 1;
    }
    epd_set_memory_area(e, x, y, x_end, y_end);
    epd_set_memory_pointer(e, x, y);
    epd_send_cmd(e, WRITE_RAM);
    /* send the image data */
    for (int j = 0; j < y_end - y + 1; j++)
        for (int i = 0; i < (x_end - x + 1) / 8; i++)
            epd_send_data(e, image_buffer[i + j * (image_width / 8)]);
}

void epd_clear_frame_memory(struct epd_s *e, unsigned char color)
{
    epd_set_memory_area(e, 0, 0, e->width - 1, e->height - 1);
    epd_set_memory_pointer(e, 0, 0);
    epd_send_cmd(e, WRITE_RAM);
    /* send the color data */
    for (int i = 0; i < e->width / 8 * e->height; i++)
        epd_send_data(e, color);
}

void epd_display_frame(struct epd_s *e)
{
    epd_send_cmd(e, DISPLAY_UPDATE_CONTROL_2);
    epd_send_data(e, 0xC4);
    epd_send_cmd(e, MASTER_ACTIVATION);
    epd_send_cmd(e, TERMINATE_FRAME_READ_WRITE);
    epd_wait_until_idle(e);
}

void epd_epaper_sleep(struct epd_s *e)
{
    epd_send_cmd(e, DEEP_SLEEP_MODE);
    epd_wait_until_idle(e);
}

const unsigned char lut_full_update[] =
{
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51, 
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

const unsigned char lut_partial_update[] =
{
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};