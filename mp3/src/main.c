#include "lcd.h"
#include "spi.h"

void app_main(void)
{
    spi_device_handle_t lcd_spi;
    spi_bus_init(LCD_WIDTH_PX* PARALLEL_LINES * BYTES_PER_PIXEL);
    spi_device_init(&lcd_spi, 10 * 1000 * 1000*3, 0, PIN_CS, 1, lcd_spi_pre_transfer_callback);
    lcd_init(lcd_spi);

    static uint8_t pixels[LCD_WIDTH_PX * LCD_HEIGHT_PX * BYTES_PER_PIXEL];
    for (int i = 0; i < LCD_WIDTH_PX * LCD_HEIGHT_PX; i++) {
        pixels[i*2] = 0x07;
        pixels[i*2+1] = 0xE0;
    }

    display_char(lcd_spi, 10, 10, 'a');
}