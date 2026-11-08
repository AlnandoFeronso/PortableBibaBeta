#include "spi.h"
#include "esp_err.h"

void spi_bus_init(spi_host_device_t host, int mosi_pin, int miso_pin, int clk_pin, int max_transfer_sz)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = miso_pin,
        .sclk_io_num = clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_transfer_sz
    };

    ESP_ERROR_CHECK(
        spi_bus_initialize(
            host,
            &bus_config,
            SPI_DMA_CH_AUTO
        )
    );
}

void spi_device_init(spi_host_device_t host, spi_device_handle_t *device, int clock_speed_hz, int mode,
                    int cs_pin, int queue_size, transaction_cb_t pre_cb)
{
    spi_device_interface_config_t device_config = {
        .clock_speed_hz = clock_speed_hz,
        .mode = mode,
        .spics_io_num = cs_pin,
        .queue_size = queue_size,
        .pre_cb = pre_cb
    };

    ESP_ERROR_CHECK(spi_bus_add_device(host, &device_config, device));
}