#pragma once
#include "driver/spi_master.h"

#define SPI_HOST_ID SPI2_HOST

void spi_bus_init(int max_transfer_sz);

void spi_device_init(spi_device_handle_t *device, int clock_speed_hz, int mode,
                    int cs_pin, int queue_size, transaction_cb_t pre_cb);