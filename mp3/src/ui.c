#include "ui.h"
#include "lcd.h"
#include "font.h"

#define TOP_BAR_HEIGHT 26
#define ALBUM_COVER_OFFSET_X 15 // offset from the left side of the display
#define ALBUM_COVER_OFFSET_Y 15  // offset from the bottom of the top bar
#define ALBUM_COVER_DIMENSIONS 140 // album covers are rectangles so this is the width and height
#define PLAY_TIME_OFFSET_Y 25 // offset from the bottom of the album cover 
#define TIME_POS_Y TOP_BAR_HEIGHT + ALBUM_COVER_OFFSET_Y + ALBUM_COVER_DIMENSIONS + PLAY_TIME_OFFSET_Y


void display_play_icon(spi_device_handle_t lcd_spi){
    draw_icon(lcd_spi, LCD_WIDTH_PX - 70, 3, stop_icon, PLAY_ICON_DIMENSIONS, PLAY_ICON_DIMENSIONS, PLAY_ICON_COLOR, 0x07E0);
}

void draw_ui(spi_device_handle_t lcd_spi){
    // TEMPORARY SO THAT WE CAN SEE THE LAYOUT 
    static uint8_t pixels[LCD_WIDTH_PX * LCD_HEIGHT_PX * BYTES_PER_PIXEL];
    for (int i = 0; i < LCD_WIDTH_PX * LCD_HEIGHT_PX; i++) {
        pixels[i*2] = 0x07;
        pixels[i*2+1] = 0xE0;
    }

    // top bar 
    lcd_draw_rect_dma(lcd_spi, 0, 0, LCD_WIDTH_PX, TOP_BAR_HEIGHT, pixels);

    // album cover
    lcd_draw_rect_dma(lcd_spi, ALBUM_COVER_OFFSET_X, TOP_BAR_HEIGHT + ALBUM_COVER_OFFSET_Y,
                      ALBUM_COVER_DIMENSIONS, ALBUM_COVER_DIMENSIONS, pixels);
    
    // play time
    lcd_draw_string(lcd_spi, ALBUM_COVER_OFFSET_X,  TIME_POS_Y, "00:00", 0x0000, 0x07E0);

    // music time
    lcd_draw_string(lcd_spi, LCD_WIDTH_PX - ALBUM_COVER_OFFSET_X - 10*5, TIME_POS_Y, "00:00", 0x0000, 0x07E0);

    // progress bar
    lcd_draw_rect_dma(lcd_spi, 80, TIME_POS_Y, 165, 16, pixels);

    // title
    lcd_draw_string(lcd_spi, 165, TOP_BAR_HEIGHT + 30, "titleleleel", 0x0000, 0x07E0);
    // author 
    lcd_draw_string(lcd_spi, 165, TOP_BAR_HEIGHT + 60, "authoror", 0x0000, 0x07E0);

    // battery icon
    lcd_draw_rect(lcd_spi, LCD_WIDTH_PX - 40, 5, 30, 15, 0xF800);

    // play icon
    display_play_icon(lcd_spi);

    // play/stop icon
}