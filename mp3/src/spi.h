#pragma once
#include "driver/spi_master.h"

void spi_bus_init(spi_host_device_t host, int mosi_pin, int miso_pin, int clk_pin, int max_transfer_sz);

void spi_device_init(spi_host_device_t host, spi_device_handle_t *device, int clock_speed_hz, int mode,
                     int cs_pin, int queue_size, transaction_cb_t pre_cb);
