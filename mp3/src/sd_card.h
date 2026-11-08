#pragma once

#define SD_SPI_HOST  SPI3_HOST
#define SD_PIN_MOSI  13
#define SD_PIN_MISO  19
#define SD_PIN_CLK   14
#define SD_PIN_CS    4 

#define SD_INIT_CLOCK_HZ (400 * 1000)

#include "spi.h"
#include <esp_err.h>

esp_err_t init_sd(spi_device_handle_t spi);
