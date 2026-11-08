#include "lcd.h"
#include "sd_card.h"
#include "spi.h"
#include "ui.h"

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

    draw_ui(lcd_spi);
}
