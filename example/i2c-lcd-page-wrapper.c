#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#include "i2c-LCD1602.h"
#include "i2c-lcd-page-wrapper.h"

/* HD44780 datasheet:
 * https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
 */


struct i2c_lcd_page i2c_lcd_page_init(struct i2c_lcd1602 i2c_lcd1602) {
	/* {{{ */
	struct i2c_lcd_page i2c_lcd = {
		.i2c_lcd1602 = i2c_lcd1602,
		.cursor_col = 0,
		.cursor_row = 0,
		.display_pos = 0,
		.row_width = 40 // See page 11 of the HD44780 datasheet
	};

	return i2c_lcd;
	/* }}} */
}


/** Clear the display, and set the cursor position to zero */
void i2c_lcd_page_clear_display(struct i2c_lcd_page *i2c_lcd_page) {
	/* {{{ */
	/* Adjust display position */
	i2c_lcd_page->display_pos = 0;

	i2c_lcd1602_clear_display(&i2c_lcd_page->i2c_lcd1602);
	/* }}} */
}


/** Move the cursor to the given x, y (column, row) coordinates. */
void i2c_lcd_page_set_cursor_pos(struct i2c_lcd_page *i2c_lcd_page, \
	uint8_t column, uint8_t row) {
	/* {{{ */
	/* See page 24 of the HD44780 datasheet */

	/* These numbers come from page 11, 21 of the HD44780 datasheet which shows
	 * how rows are really just treated as higher-number columns, with each
	 * row containing 40 columns and row 2 starting at 0x40 */
	int row_offsets[] = { 0x0, 0x40 };

	uint8_t ac = i2c_lcd_page->display_pos + column + row_offsets[row];

	/* Adjust the cursor coordinates */
	i2c_lcd_page->cursor_col = column;
	i2c_lcd_page->cursor_row = row;

	i2c_lcd1602_set_cursor_pos(&i2c_lcd_page->i2c_lcd1602, ac);
	/* }}} */
}


/** Shift the screen or the cursor to the right or to the left */
void i2c_lcd_page_shift(struct i2c_lcd_page *i2c_lcd_page, char screen_cursor,
	char right_left) {
	/* {{{ */
	/* If it is the cursor being moved, adjust the cursor coordinates */
	if (screen_cursor == 0) {
		if (right_left == 1) {
			i2c_lcd_page->cursor_col++;
		} else {
			i2c_lcd_page->cursor_col--;
		}
	/* If it is the screen that is being moved, adjust the display position */
	} else if (screen_cursor == 1) {
		if (right_left == 1) {
			i2c_lcd_page->display_pos--;
			i2c_lcd_page->cursor_col++;
		} else {
			i2c_lcd_page->display_pos++;
			i2c_lcd_page->cursor_col--;
		}
	}

	i2c_lcd1602_shift(&i2c_lcd_page->i2c_lcd1602, screen_cursor, right_left);
	/* }}} */
}


void i2c_lcd_page_send_char(struct i2c_lcd_page *i2c_lcd_page, char c) {
	/* {{{ */
	/* If the LCD is NOT set to shift the whole display (as well as the cursor)
	 * after receiving a character ... */
	if (i2c_lcd_page->i2c_lcd1602.entry_shift == 0) {
		/* If the LCD is set to shift the cursor to the right after receiving
		 * a character ... */
		if (i2c_lcd_page->i2c_lcd1602.entry_shift_increment == LCD_ENTRYINCREMENT) {
			i2c_lcd_page->cursor_col++;
		} else if (i2c_lcd_page->i2c_lcd1602.entry_shift_increment == LCD_ENTRYDECREMENT) {
			i2c_lcd_page->cursor_col--;
		}
	} else if (i2c_lcd_page->i2c_lcd1602.entry_shift == 1) {
		/* If the LCD is set to shift the display to the right after receiving
		 * a character ... */
		if (i2c_lcd_page->i2c_lcd1602.entry_shift_increment == LCD_ENTRYINCREMENT) {
			i2c_lcd_page->display_pos++;
		} else if (i2c_lcd_page->i2c_lcd1602.entry_shift_increment == LCD_ENTRYDECREMENT) {
			i2c_lcd_page->display_pos--;
		}
	}

	i2c_lcd1602_send_char(&i2c_lcd_page->i2c_lcd1602, c);
	/* }}} */
}
