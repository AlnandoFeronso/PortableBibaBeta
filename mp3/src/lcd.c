#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "lcd.h"
#include "spi.h"
#include "font.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "esp_err.h"

#define PIN_DC  26
#define PIN_RST 27
#define PIN_BL  33

#define LCD_BK_LIGHT_ON_LEVEL 1

// The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; // No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

DRAM_ATTR static const lcd_init_cmd_t ili_init_cmds[] = {
    // Power Control B
    // Power control = 0, DC_ENA = 1
    {0xCF, {0x00, 0x83, 0x30}, 3},
    // Power On Sequence Control
    // cp1 keeps 1 frame, 1st frame enable
    // vcl = 0, ddvdh=3, vgh=1, vgl=2
    // DDVDH_ENH=1
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
    // Driver Timing Control A
    // non-overlap=default +1
    // EQ=default - 1, CR=default
    // pre-charge=default - 1
    {0xE8, {0x85, 0x01, 0x79}, 3},
    // Power Control A
    // VCORE = 1.6V, DDVDH = 5.6V
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    // Pump Ratio Control
    // DDVDH = 2 * VCL
    {0xF7, {0x20}, 1},
    // Driver Timing Control B
    // all=0 unit
    {0xEA, {0x00, 0x00}, 2},
    // Power Control 1
    // GVDD=4.75V
    {0xC0, {0x26}, 1},
    // Power Control 2
    // DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3
    {0xC1, {0x11}, 1},
    // VCOM Control 1
    // VCOMH=4.025V, VCOML=-0.950V
    {0xC5, {0x35, 0x3E}, 2},
    // VCOM Control 2
    // VCOMH=VMH-2, VCOML=VML-2
    {0xC7, {0xBE}, 1},
    // Memory Access Control
    // MV = 1 - landscape orientation, BGR = 1 - BGR color order
    {0x36, {0x28}, 1},
    // Pixel Format Set
    // 16 bits/pixel
    {0x3A, {0x55}, 1},
    // Frame Rate Control
    // DIVA = 00 - fosc / 1
    // RTNA = 11011 - 27 clocks per line
    // Approximately 70 Hz under datasheet conditions
    {0xB1, {0x00, 0x1B}, 2},
    // Enable 3G
    // 3 gamma control disabled
    {0xF2, {0x08}, 1},
    // Gamma Set
    // Select gamma curve 1
    {0x26, {0x01}, 1},
    // Positive Gamma Correction
    {0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
    // Negative gamma correction
    {0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
    // Column address set, SC=0, EC=0xEF 
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    // Page address set, SP=0, EP=0x013F
    {0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
    // Memory write
    {0x2C, {0}, 0},
    // Entry Mode Set
    // GAS = 1 - low-voltage detection disabled
    // GON = 1, DTE = 1 - normal display
    {0xB7, {0x07}, 1},
    // Display Function Control
    // PTG = 10 - interval scan mode in non-display area
    // PT  = 10 - set source and VCOM outputs to AGND in non-display area in partial display mode
    // REV = 1 - normally white lcd
    // GS  = 0 - gate scan direction G1 -> G320
    // SS  = 0 - shift direction of outputs from the source driver S1 -> S720
    // SM  = 0 - gate driver pin arrangement (GS = 0 and SS = 0 so it is G1->G2->...->G319->G320)
    // ISC = 0010 - scan interval of 5 frames
    // NL  = 100111 - 320 LCD drive lines
    // PCDIV = 000000 - DOTCLK division ratio = 2, not used for SPI communication 
    {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
    // Sleep Out
    {0x11, {0}, 0x80},
    // Display On
    {0x29, {0}, 0x80},
    // End of initialization commands
    {0, {0}, 0xFF},
};

// Send a command to the LCD. Uses spi_device_polling_transmit, which waits
// until the transfer is complete.
//
// Since command transactions are usually small, they are handled in polling
// mode for higher speed. The overhead of interrupt transactions is more than
// just waiting for the transaction to complete.

static void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,        // Command is 8 bits
        .tx_buffer = &cmd,  // The data is the cmd itself
        .user = (void *)0   // D/C needs to be set to 0
    };

    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

// Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
// transfer is complete.
//
// Since data transactions are usually small, they are handled in polling
// mode for higher speed. The overhead of interrupt transactions is more than
// just waiting for the transaction to complete.

static void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    if (len <= 0) {
        return;
    }
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
        .user = (void *)1
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

// This function is called (in irq context!) just before a transmission starts. It will
// set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(PIN_DC, dc);
}

// Initialize the display
void lcd_init(spi_device_handle_t spi)
{
    int cmd = 0;

    // Initialize non-SPI GPIOs
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = ((1ULL << PIN_DC) | (1ULL << PIN_RST) | (1ULL << PIN_BL));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    // Reset the display
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // detect LCD type
    const lcd_init_cmd_t* lcd_init_cmds = ili_init_cmds;

    // Send all the commands
    while (lcd_init_cmds[cmd].databytes != 0xff) {
        lcd_cmd(spi, lcd_init_cmds[cmd].cmd);
        lcd_data(spi, lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes & 0x1F);
        if (lcd_init_cmds[cmd].databytes & 0x80) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        cmd++;
    }

    ///Enable backlight
    gpio_set_level(PIN_BL, LCD_BK_LIGHT_ON_LEVEL);
}

static void lcd_set_window(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4];

    // Set the range of columns to draw to
    data[0] = x1 >> 8;
    data[1] = x1 & 0xFF;
    data[2] = x2 >> 8;
    data[3] = x2 & 0xFF;

    lcd_cmd(spi, 0x2A);
    lcd_data(spi, data, 4);

    // Set the range of rows to draw to
    data[0] = y1 >> 8;
    data[1] = y1 & 0xFF;
    data[2] = y2 >> 8;
    data[3] = y2 & 0xFF;

    lcd_cmd(spi, 0x2B);
    lcd_data(spi, data, 4);

    // Start writing pixel data
    lcd_cmd(spi, 0x2C);
}

void lcd_draw_rect(spi_device_handle_t spi, uint16_t x, uint16_t y,
                   uint16_t width, uint16_t height, uint16_t color)
{
    size_t pixel_count = (size_t)width * height;
    size_t data_size = pixel_count * BYTES_PER_PIXEL;
    
    uint8_t *pixels = malloc(data_size);

    // Fill the buffer with the selected color
    for (size_t i = 0; i < pixel_count; i++) {
        pixels[i*2] = color >> 8;
        pixels[i*2+1] = color & 0xFF;
    }

    lcd_set_window(spi, x, y, x + width - 1, y + height - 1);
    lcd_data(spi, pixels, data_size);

    free(pixels);
}

static void send_lines(spi_device_handle_t spi, const uint8_t *linedata, size_t data_size)
{
    static spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));

    trans.length = data_size*8;
    trans.tx_buffer = linedata;
    trans.user = (void*)1;

    ESP_ERROR_CHECK(spi_device_queue_trans(spi, &trans, portMAX_DELAY));
}

static void send_line_finish(spi_device_handle_t spi)
{
    spi_transaction_t *rtrans;
    // Wait until DMA finishes sending the previous data
    ESP_ERROR_CHECK(spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY));
}

void lcd_draw_rect_dma(spi_device_handle_t spi, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height, const uint8_t *pixels)
{
    uint8_t *lines[2];

    size_t line_buffer_size = width * PARALLEL_LINES * BYTES_PER_PIXEL;

    // Allocate memory for the pixel buffers
    for (int i = 0; i < 2; i++) {
        lines[i] = spi_bus_dma_memory_alloc(LCD_SPI_HOST, line_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        assert(lines[i] != NULL);
    }

    // Set the drawing area
    lcd_set_window(spi, x, y, x + width-1, y+height-1);

    int sending_line = -1;
    int calc_line = 0;

    for (int line_offset = 0; line_offset < height; line_offset += PARALLEL_LINES) {
        // Calculate the number of lines to send
        int lines_to_send = height - line_offset;

        if (lines_to_send > PARALLEL_LINES) {
            lines_to_send = PARALLEL_LINES;
        }

        size_t line_data_size = width * lines_to_send * BYTES_PER_PIXEL;
        // Copy the pixel data to the DMA buffer
        memcpy(lines[calc_line], pixels+line_offset*width*BYTES_PER_PIXEL, line_data_size);

        // Finish up the sending process of the previous line, if any
        if (sending_line != -1) {
            send_line_finish(spi);
        }

        // Swap sending_line and calc_line
        sending_line = calc_line;
        calc_line = (calc_line == 1) ? 0 : 1;

        // Send the line we currently calculated
        send_lines(spi, lines[sending_line], line_data_size);
    }
    // Wait for the last transfer to finish
    send_line_finish(spi);

    // Free the allocated pixel buffers
    for (int i = 0; i < 2; i++) {
        free(lines[i]);
    }
}

void lcd_draw_char(spi_device_handle_t spi, uint16_t x, uint16_t y,
                   char c, uint16_t fg, uint16_t bg)
{
    const uint8_t *glyph = font[(uint8_t)c];

    uint8_t buf[FONT_WIDTH * FONT_HEIGHT * BYTES_PER_PIXEL];
    uint8_t *p = buf;

    for (int row = 0; row < FONT_HEIGHT; row++){ 
        uint16_t bits = glyph[2 * row + 1] << 8;
        bits |= glyph[2 * row];

        for (int col = 0; col < FONT_WIDTH; col++) {
            uint16_t color = (bits & (1 << (FONT_WIDTH - 1 - col))) ? fg : bg;
            *p++ = color >> 8;
            *p++ = color & 0xFF;
        }
    }

    lcd_set_window(spi, x, y, x + FONT_WIDTH - 1, y + FONT_HEIGHT - 1);
    lcd_data(spi, buf, sizeof(buf));
}

void lcd_draw_string(spi_device_handle_t spi, uint16_t x, uint16_t y,
                     const char *s, uint16_t fg, uint16_t bg)
{
    for (; *s != '\0'; s++) {
        if (x + FONT_WIDTH > LCD_WIDTH_PX) {
            break;
        }
        lcd_draw_char(spi, x, y, *s, fg, bg);
        x += FONT_WIDTH;
    }
}

void draw_icon(spi_device_handle_t spi, uint16_t x, uint16_t y,
               const uint8_t *icon, const int icon_width, const int icon_height,
               uint16_t fg, uint16_t bg)
    {
    int byte_num = icon_width / 8;
    if (icon_width%8!=0){
        byte_num++;
    }
    
    uint8_t buf[icon_width * icon_height * BYTES_PER_PIXEL];
    uint8_t *p = buf;

    for (int row = 0; row < icon_height; row++){ 
        uint32_t bits = icon[byte_num * row + 2] << 16;
        bits |= icon[byte_num * row + 1] << 8;
        bits |= icon[byte_num * row];

        for (int col = 0; col < icon_width; col++) {
            uint16_t color = (bits & (1 << (icon_width - 1 - col))) ? fg : bg;
            *p++ = color >> 8;
            *p++ = color & 0xFF;
        }
    }

    lcd_set_window(spi, x, y, x + icon_width - 1, y + icon_height - 1);
    lcd_data(spi, buf, sizeof(buf));
}