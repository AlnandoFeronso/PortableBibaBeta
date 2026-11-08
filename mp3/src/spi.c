#include "spi.h"
#include "esp_err.h"

#define PIN_MOSI 23
#define PIN_MISO -1 // We don't need to read from the lcd 
#define PIN_CLK 18

void spi_bus_init(int max_transfer_sz)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_transfer_sz
    };

    ESP_ERROR_CHECK(
        spi_bus_initialize(
            SPI_HOST_ID,
            &bus_config,
            SPI_DMA_CH_AUTO
        )
    );
}

void spi_device_init(spi_device_handle_t *device, int clock_speed_hz, int mode,
                    int cs_pin, int queue_size, transaction_cb_t pre_cb)
{
    spi_device_interface_config_t device_config = {
        .clock_speed_hz = clock_speed_hz,
        .mode = mode,
        .spics_io_num = cs_pin,
        .queue_size = queue_size,
        .pre_cb = pre_cb
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_ID, &device_config, device));
}