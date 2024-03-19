#ifndef I2C_LCD_PAGE_WRAPPER
#define I2C_LCD_PAGE_WRAPPER

#include <stdint.h>
#include <stddef.h>

#include "i2c-LCD1602.h"


struct i2c_lcd_page {
	struct i2c_lcd1602 i2c_lcd1602;
	uint8_t cursor_col;
	uint8_t cursor_row;
	uint8_t display_pos;
	uint8_t row_width;
};


struct i2c_lcd_page i2c_lcd_page_init(struct i2c_lcd1602 i2c_lcd1602);

void i2c_lcd_page_clear_display(struct i2c_lcd_page *i2c_lcd_page);

void i2c_lcd_page_set_cursor_pos(struct i2c_lcd_page *i2c_lcd_page, uint8_t column, uint8_t row);

void i2c_lcd_page_shift(struct i2c_lcd_page *i2c_lcd_page, char screen_cursor, char right_left);

void i2c_lcd_page_send_char(struct i2c_lcd_page *i2c_lcd_page, char c);

#endif
