
#pragma once

#include <stdint.h>
#include "driver/spi_master.h"

#define LCD_SPI_HOST  SPI2_HOST
#define LCD_PIN_MOSI  23
#define LCD_PIN_MISO  -1 // We don't need to read from the LCD
#define LCD_PIN_CLK   18
#define LCD_PIN_CS    25

#define LCD_WIDTH_PX  320
#define LCD_HEIGHT_PX 240

#define BYTES_PER_PIXEL 2

// To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
// but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

void lcd_spi_pre_transfer_callback(spi_transaction_t *t);

void lcd_init(spi_device_handle_t spi);

// LCD drawing functions
void lcd_draw_rect(spi_device_handle_t spi, uint16_t x, uint16_t y,
                   uint16_t width, uint16_t height, uint16_t color);

void lcd_draw_rect_dma(spi_device_handle_t spi, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height, const uint8_t *pixels);

void lcd_draw_char(spi_device_handle_t spi, uint16_t x, uint16_t y,
                   char c, uint16_t fg, uint16_t bg);

void lcd_draw_string(spi_device_handle_t spi, uint16_t x, uint16_t y,
                     const char *s, uint16_t fg, uint16_t bg);

void draw_icon(spi_device_handle_t spi, uint16_t x, uint16_t y,
               const uint8_t *icon, const int icon_width, const int icon_height,
               uint16_t fg, uint16_t bg);