#ifndef ILI9341_H_INCLUDED
#define ILI9341_H_INCLUDED
#include <sam3x8e.h>
#include <pio.h>
#include <pmc.h>
#include <delay.h>
#include <spi_master.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ff.h>

#define ILI9341_ORIENTATION_LANDSCAPE 3
#define ILI9341_ORIENTATION_VERTICAL 0

#define ILI9341_TFTWIDTH  240
#define ILI9341_TFTHEIGHT 320

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0A
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF  0x20
#define ILI9341_INVON   0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29

#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_RAMRD   0x2E

#define ILI9341_PTLAR   0x30
#define ILI9341_PIXFMT  0x3A

#define ILI9341_MADCTL  0x36
#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1
/*
#define ILI9341_PWCTR6  0xFC

*/

// Color definitions
#define	ILI9341_BLACK   0x0000
#define	ILI9341_BLUE    0x001F
#define	ILI9341_RED     0xF800
#define	ILI9341_GREEN   0x07E0
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW  0xFFE0  
#define ILI9341_WHITE   0xFFFF
//Basic Colors
#define RED		0xf800
#define GREEN	0x07e0
#define BLUE	0x001f
#define BLACK	0x0000
#define YELLOW	0xffe0
#define WHITE	0xffff

//Other Colors
#define CYAN		0x07ff	
#define BRIGHT_RED	0xf810	
#define GRAY1		0x8410  
#define GRAY2		0x4208  

//TFT resolution 240*320
static uint32_t prv_width = ILI9341_TFTWIDTH;       //true while rotation = 0
static uint32_t prv_height = ILI9341_TFTHEIGHT;      //true while rotation = 0
#define MIN_X	0
#define MIN_Y	0
#define MAX_X   (prv_width - 1)
#define MAX_Y	(prv_height - 1)

//Font parameters
#define FONT_SPACE 6
#define FONT_X 8
#define FONT_Y 8

#define DC_PIN                    (1 << 14)
#define DC_PORT                   PIOA
#define DC_PORT_ID                ID_PIOA
 
#define RESET_PIN                 (1 << 15)
#define RESET_PORT                PIOA
#define RESET_PORT_ID             ID_PIOA

#define SPI_ILI9341                SPI0
#define SPI_DEVICE_ILI9341_ID     (0x1)
#define SPI_ILI9341_BAUDRATE       8400000

#define BUFFPIXEL 60

static uint32_t rotation = 0;

void ILI9341_command(uint8_t c);
void ILI9341_data(uint8_t c);
void ILI9341_data16(uint16_t c);
void ILI9341_init(int rot);
uint8_t ILI9341_read_register(uint8_t addr, uint8_t x_parameter);
uint8_t ILI9341_read_ID(void);
void ILI9341_set_rotation(uint32_t m);
void ILI9341_set_col(uint16_t StartCol, uint16_t EndCol);
void ILI9341_set_page(uint16_t StartPage, uint16_t EndPage);
void ILI9341_set_xy(uint16_t poX, uint16_t poY);
void ILI9341_set_pixel(uint16_t poX, uint16_t poY, uint16_t color);
void ILI9341_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void ILI9341_draw_horizontal(uint16_t poX, uint16_t poY, uint16_t length, uint16_t color);
void ILI9341_draw_vertical(uint16_t poX, uint16_t poY, uint16_t length, uint16_t color);
void ILI9341_draw_char(uint8_t ascii, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor);
void ILI9341_draw_string(char *string, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor);
void ILI9341_fill_rectangle(uint16_t poX, uint16_t poY, uint16_t length, uint16_t width, uint16_t color);
void ILI9341_fill_screen(void);
void ILI9341_fill_screen_xy(uint16_t XL, uint16_t XR, uint16_t YU, uint16_t YD, uint16_t color);
uint16_t ILI9341_constrain(uint16_t input, uint16_t min, uint16_t max);
uint16_t ILI9341_read16(FIL *f);
uint32_t ILI9341_read32(FIL *f);
uint8_t ILI9341_read_header(FIL *f);
void ILI9341_bmp_draw(FIL *f, uint16_t x, uint16_t y);
#endif