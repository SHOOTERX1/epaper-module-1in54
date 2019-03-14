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
#if !defined(EPAPER_CORE_H)
#define EPAPER_CORE_H

#include <stddef.h>

// Epaper commands
#define DRIVER_OUTPUT_CONTROL                       0x01
#define BOOSTER_SOFT_START_CONTROL                  0x0C
#define GATE_SCAN_START_POSITION                    0x0F
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x1A
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_VCOM_REGISTER                         0x2C
#define WRITE_LUT_REGISTER                          0x32
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_TIME                               0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define TERMINATE_FRAME_READ_WRITE                  0xFF

// Display resolution
#define EPD_WIDTH       200
#define EPD_HEIGHT      200

#define EPAPER_SPI_DEV_PATH "/dev/epaper_spi_dev"

#define EPD_BUSY 1
#define EPD_IDLE 0

struct epd_s {
    int fd;
    int width;
    int height;
};

extern const unsigned char lut_full_update[];
extern const unsigned char lut_partial_update[];

struct epd_s *epd_create_epaper(void);
void epd_release_epaper(struct epd_s *e);
void epd_delay_ms(unsigned int ms);
void epd_init_epaper(struct epd_s *e, const unsigned char *l);
size_t epd_spi_transfer(struct epd_s *e, const char c);
int epd_send_data(struct epd_s *e, const char c);
int epd_send_cmd(struct epd_s *e, const char c);
int epd_wait_until_idle(struct epd_s *e);
void epd_reset(struct epd_s *e);
void epd_set_frame_memory(
    struct epd_s *e,
    const unsigned char *image_buffer,
    int x,
    int y,
    int image_width,
    int image_height
);
void epd_clear_frame_memory(struct epd_s *e, unsigned char color);
void epd_display_frame(struct epd_s *e);
void epd_epaper_sleep(struct epd_s *e);

#endif // EPAPER_CORE_H
