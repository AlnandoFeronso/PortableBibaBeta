#include "lcd.h"
#include "sd_card.h"
#include "spi.h"

void app_main(void)
{
    spi_device_handle_t lcd_spi;
    spi_bus_init(LCD_SPI_HOST, LCD_PIN_MOSI, LCD_PIN_MISO, LCD_PIN_CLK,
                 LCD_WIDTH_PX * PARALLEL_LINES * BYTES_PER_PIXEL);
    spi_device_init(LCD_SPI_HOST, &lcd_spi, 10 * 1000 * 1000 * 3, 0, LCD_PIN_CS, 1,
                    lcd_spi_pre_transfer_callback);
    lcd_init(lcd_spi);

    spi_device_handle_t sd_spi;
    spi_bus_init(SD_SPI_HOST, SD_PIN_MOSI, SD_PIN_MISO, SD_PIN_CLK, 4096);
    spi_device_init(SD_SPI_HOST, &sd_spi, SD_INIT_CLOCK_HZ, 0, SD_PIN_CS, 1, NULL);

    static uint8_t pixels[LCD_WIDTH_PX * LCD_HEIGHT_PX * BYTES_PER_PIXEL];
    for (int i = 0; i < LCD_WIDTH_PX * LCD_HEIGHT_PX; i++) {
        pixels[i*2] = 0x07;
        pixels[i*2+1] = 0xE0;
    }

    lcd_draw_rect_dma(lcd_spi, 0, 0, LCD_WIDTH_PX, LCD_HEIGHT_PX, pixels);
    
    lcd_draw_char(lcd_spi, 10, 10, 'a', 0x0000, 0x07E0);
    lcd_draw_string(lcd_spi, 10, 30, "Hello, MP3!", 0x0000, 0x07E0);
}
