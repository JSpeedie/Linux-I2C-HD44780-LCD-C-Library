#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#include "i2c-LCD1602.h"

/* HD44780 datasheet:
 * https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
 */


struct i2c_lcd1602 i2c_lcd1602_init(int i2c_lcd_fd, uint8_t periph_addr,
	uint8_t columns, uint8_t rows, uint8_t dotsize, uint8_t backlight) {
	/* {{{ */

	struct i2c_lcd1602 ret = {
		.fd = i2c_lcd_fd,
		.address = periph_addr,
		.columns = columns,
		.rows = rows,
		.dotsize = 0 // 5x8 dotsize
	};

	if (backlight == 1) ret.backlight = LCD_BACKLIGHT;
	else ret.backlight = LCD_NOBACKLIGHT;

	return ret;
	/* }}} */
}


/* See page 46 of the HD44780 datasheet for 4-bit initialization */
void i2c_lcd1602_begin(struct i2c_lcd1602 *i2c_lcd1602) {

	/* According to the HD44780 datasheet, when the display powers up it is
	 * configured as follows:
	 *
	 * 1. Display clear
	 * 2. Function set:
	 *    DL = 1; 8-bit interface data
	 *    N = 0; 1-line display
	 *    F = 0; 5x8 dot character font
	 * 3. Display on/off control:
	 *    D = 0; Display off
	 *    C = 0; Cursor off
	 *    B = 0; Blinking off
	 * 4. Entry mode set:
	 *    I/D = 1; Increment by 1
	 *    S = 0; No shift
	 * This function exists then to change those values to what could be
	 * common-case defaults.
	 */


	/* According to page 46 of the HD44780 datasheet, we must wait for 40ms
	 * after the Vcc reaches 2.7 V before sending commands. */
	/* Sleep for 40ms */
	struct timespec a = { .tv_sec = 0, .tv_nsec = 40000000};
	nanosleep(&a, NULL);

	/* Set the functionality of the LCD (E.g. here: 4-bit operation . 2 display
	 * lines, font 0 (i.e. 5x8 dots) */
	i2c_lcd1602_function_set(i2c_lcd1602, 4, 2, 0);

	/* Set the display control of the LCD  (E.g. here: display = on,
	 * cursor = on, cursor blinking = on) */
	i2c_lcd1602_display_control(i2c_lcd1602, 1, 1, 1);

	/* Clear the display */
	 i2c_lcd1602_clear_display(i2c_lcd1602);

	/* Set the LCD to have new characters appear left-to-right, and do not
	 * shift the display upon receiving a new character */
	i2c_lcd1602_entry_mode_set(i2c_lcd1602, 1, 0);

	/* Move the cursor back to the beginning */
	i2c_lcd1602_cursor_home(i2c_lcd1602);
}


/** Clear the display, and set the cursor position to zero */
void i2c_lcd1602_clear_display(struct i2c_lcd1602 *i2c_lcd1602) {
	/* {{{ */
	/* See page 28 of the HD44780 datasheet */

	/* Set DB7 to DB0 appropriately */
	uint8_t data = LCD_CLEARDISPLAY; // 00000001
	/* Set RS and R/W appropriately */
	uint8_t mode = set_mode(0, 0);
	/* Respect backlight settings for the LCD */
	mode |= i2c_lcd1602->backlight;

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	// TODO:
	/* See page 24 of the HD44780 datasheet. No time is listed for this
	 * instruction. */
	/* Sleep for 2000µs */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 2000000};
	nanosleep(&a, NULL);
	/* }}} */
}


/** Set the cursor position */
void i2c_lcd1602_set_cursor_pos(struct i2c_lcd1602 *i2c_lcd1602, uint8_t ac) {
	/* {{{ */
	/* See page 21, 24 of the HD44780 datasheet */

	/* Set the command type */
	uint8_t data = LCD_SETDDRAMADDR;
	data |= ac;
	/* Set RS and R/W appropriately */
	uint8_t mode = set_mode(0, 0);
	/* Respect backlight settings for the LCD */
	mode |= i2c_lcd1602->backlight;

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	/* According to page 24 of the HD44780 datasheet, this operation takes a
	 * maximum of 37µs */
	/* Sleep for 37µs */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 37000};
	nanosleep(&a, NULL);
	/* }}} */
}


/** Move the cursor to (0, 0) */
void i2c_lcd1602_cursor_home(struct i2c_lcd1602 *i2c_lcd1602) {
	/* {{{ */
	/* See page 24 of the HD44780 datasheet */

	/* Set DB7 to DB0 appropriately */
	uint8_t data = LCD_RETURNHOME;
	/* Set RS and R/W appropriately */
	uint8_t mode = set_mode(0, 0);
	/* Respect backlight settings for the LCD */
	mode |= i2c_lcd1602->backlight;

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	/* According to page 24 of the HD44780 datasheet, this operation takes a
	 * maximum of 1.52ms */
	/* Sleep for 1.52ms */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 1520000};
	nanosleep(&a, NULL);
	/* }}} */
}


/** Set the entry mode for the i2c LCD */
void i2c_lcd1602_entry_mode_set(struct i2c_lcd1602 *i2c_lcd1602, uint8_t
	increment, uint8_t shift) {
	/* {{{ */
	/* See page 24, 26, 40, 42 of the HD44780 datasheet */

	/* Set the command type */
	uint8_t data = LCD_ENTRYMODESET;
	/* Set the details of the command, i.e., set the entry mode
	 * - Increment (have the cursor move to the right after receiving a new
	 *   character.
	 * - Decrement (have the cursor move to the left after receiving a new
	 *   character.
	 * - Shift (move the whole display in the direction of the cursor moves
	 *   (which depends on whether increment or decrement is set) after
	 *   receiving a new character).
	 * - No shift (do NOT move the whole display in the direction that the
	 *   cursor moves (which depends on whether increment or decrement is set)
	 *   after receiving a new character).
	 */
	if (increment == 1) {
		data |= LCD_ENTRYINCREMENT;
		i2c_lcd1602->entry_shift_increment = 1;
	} else if (increment == 0) {
		data |= LCD_ENTRYDECREMENT;
		i2c_lcd1602->entry_shift_increment = 0;
	}
	if (shift == 1) {
		data |= LCD_ENTRYSHIFT;
		i2c_lcd1602->entry_shift = 1;
	} else if (shift == 0) {
		data |= LCD_ENTRYNOSHIFT;
		i2c_lcd1602->entry_shift = 0;
	}
	/* Set both RS and R/W to 0 */
	uint8_t mode = set_mode(0, 0);

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	/* According to page 24 of the HD44780 datasheet, this operation takes a
	 * maximum of 37µs */
	/* Sleep for 37µs */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 37000};
	nanosleep(&a, NULL);
	/* }}} */
}


/** Set the display controls for the i2c LCD */
void i2c_lcd1602_display_control(struct i2c_lcd1602 *i2c_lcd1602,
	uint8_t display, uint8_t cursor, uint8_t cursorblinking) {
	/* {{{ */
	/* See page 24, 42 of the HD44780 datasheet */

	/* Set the command type */
	uint8_t data = LCD_DISPLAYONOFFCONTROL;
	/* Set the details of the command, i.e., set the display settings */
	if (display == 1) data |= LCD_DISPLAYON;
	else if (display == 0) data |= LCD_DISPLAYOFF;
	if (cursor == 1) data |= LCD_CURSORON;
	else if (cursor == 0) data |= LCD_CURSOROFF;
	if (cursorblinking == 1) data |= LCD_BLINKON;
	else if (cursorblinking == 0) data |= LCD_BLINKOFF;
	/* Set both RS and R/W to 0 */
	uint8_t mode = set_mode(0, 0);
	/* Respect backlight settings for the LCD */
	mode |= i2c_lcd1602->backlight;

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	/* According to page 24 of the HD44780 datasheet, this operation takes a
	 * maximum of 37µs */
	/* Sleep for 37µs */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 37000};
	nanosleep(&a, NULL);
	/* }}} */
}


/** Shift the screen or the cursor to the right or to the left */
void i2c_lcd1602_shift(struct i2c_lcd1602 *i2c_lcd1602, uint8_t screen_cursor,
	uint8_t right_left) {
	/* {{{ */
	/* See page 24, 29 of the HD44780 datasheet */

	/* Set the data */
	uint8_t data = LCD_CURSORDISPLAYSHIFT; // 00010000
	if (screen_cursor == 1) data |= LCD_DISPLAYMOVE;
	else if (screen_cursor == 0) data |= LCD_CURSORMOVE;
	if (right_left == 1) data |= LCD_MOVERIGHT;
	else if (right_left == 0) data |= LCD_MOVELEFT;
	/* Set RS and R/W appropriately */
	uint8_t mode = set_mode(0, 0);
	/* Respect backlight settings for the LCD */
	mode |= i2c_lcd1602->backlight;

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	/* According to page 24 of the HD44780 datasheet, this operation takes a
	 * maximum of 37µs */
	/* Sleep for 37µs */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 37000};
	nanosleep(&a, NULL);
	/* }}} */
}


/** Establish the functionality settings for the i2c LCD */
void i2c_lcd1602_function_set(struct i2c_lcd1602 *i2c_lcd1602,
	uint8_t data_length, uint8_t display_lines, uint8_t font) {
	/* {{{ */
	/* See page 24, 27, 29 of the HD44780 datasheet */

	/* Set the command type */
	uint8_t data = LCD_FUNCTIONSET; // 00100000
	/* Set the details of the command, i.e. ... */
	/* ... Set the interface data length ... */
	if (data_length == 8) data |= LCD_8BITMODE;
	else if (data_length == 4) data |= LCD_4BITMODE;
	/* ... Set the number of display lines ... */
	if (display_lines == 2) data |= LCD_2LINE;
	else if (display_lines == 1) data |= LCD_1LINE;
	/* ... and the character font */
	if (font == 1) data |= LCD_5x10DOTS;
	else if (font == 0) data |= LCD_5x8DOTS;
	/* Set both RS and R/W to 0 */
	uint8_t mode = set_mode(0, 0);

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	/* According to page 24 of the HD44780 datasheet, this operation takes a
	 * maximum of 37µs */
	/* Sleep for 37µs */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 37000};
	nanosleep(&a, NULL);
	/* }}} */
}


void i2c_lcd1602_send_char(struct i2c_lcd1602 *i2c_lcd1602, char c) {
	/* {{{ */
	/* See page 25 of the HD44780 datasheet */

	/* Set the data */
	uint8_t data = c;
	/* Set RS and R/W appropriately */
	uint8_t mode = set_mode(1, 0);
	/* Respect backlight settings for the LCD */
	mode |= i2c_lcd1602->backlight;

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	/* According to page 25 of the HD44780 datasheet, this operation takes a
	 * maximum of 37µs + 4µs */
	/* Sleep for 41µs */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 41000};
	nanosleep(&a, NULL);
	/* }}} */
}


/** Set the backlight setting for the LCD */
void i2c_lcd1602_set_backlight(struct i2c_lcd1602 *i2c_lcd1602, uint8_t
	backlight) {
	/* {{{ */
	// TODO:
	/* See page 24, 26, 40, 42 of the HD44780 datasheet */

	/* There is no "set backlight" command for the LCD. Instead, the backlight
	 * is set on or off with each sent command. That said, we can change the
	 * stored value for the backlight that exists in the LCD struct, and use
	 * the entry mode set command as a means of immediately bringing about
	 * that change without changing anything else (i.e. without sending a
	 * character or shifting the display) */

	/* Set the command type */
	uint8_t data = LCD_ENTRYMODESET;
	/* Maintain the LCD  entry mode settings */
	data |= i2c_lcd1602->entry_shift_increment;
	data |= i2c_lcd1602->entry_shift;
	/* Set both RS and R/W to 0 */
	uint8_t mode = set_mode(0, 0);

	i2c_lcd1602_write_4bitmode(i2c_lcd1602, data, mode);

	/* According to page 24 of the HD44780 datasheet, this operation takes a
	 * maximum of 37µs */
	/* Sleep for 37µs */
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 37000};
	nanosleep(&a, NULL);
	/* }}} */
}


/** Write to the i2c LCD in 4 bit mode. The real difference between writing
 * and reading in 4 bit vs. 8 bit mode is that two 4 bit instructions are
 * used to accomplish what would be accomplished in one 8 bit instruction.
 * Compare the last stages of page 45 and page 46 of the HD44780 datasheet
 * to see what I mean.
 */
void i2c_lcd1602_write_4bits(struct i2c_lcd1602 *i2c_lcd1602, uint8_t
	data_and_mode) {

	/* =================
	 * with enable bit stuff
	 * ================= */

	write(i2c_lcd1602->fd, &data_and_mode, 1);

	// TODO: switch this to the minimum delay necessary (1µs)?
	struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 2000000};
	nanosleep(&a, NULL);

	uint8_t data_and_mode_and_enable = data_and_mode | E;
	write(i2c_lcd1602->fd, &data_and_mode_and_enable, 1);

	// TODO: return to the minimum delay necessary
	// /* According to page 49 of the HD44780 datasheet, ______ takes
	// at most 1µs (1000ns) */
	// /* Sleep for 1µs */
	// struct timespec a = (struct timespec) { .tv_sec = 0, .tv_nsec = 1000};
	// nanosleep(&a, NULL);

	// TODO: temporary "long" delay for testing purposes
	/* Sleep for 2ms */
	a = (struct timespec) { .tv_sec = 0, .tv_nsec = 2000000};
	nanosleep(&a, NULL);

	uint8_t data_and_mode_and_disable = data_and_mode & ~E;
	write(i2c_lcd1602->fd, &data_and_mode_and_disable, 1);

	/* Sleep for 37µs */
	a = (struct timespec) { .tv_sec = 0, .tv_nsec = 37000};
	nanosleep(&a, NULL);
}


/** Send an instruction to the i2c LCD in 4 bit mode. The real difference
 * between doing so in 4 bit mode vs. 8 bit mode is that two 4 bit instructions
 * are used to accomplish what would be accomplished in one 8 bit instruction.
 * Compare the last stages of page 45 and page 46 of the HD44780 datasheet to
 * see what I mean.
 */
void i2c_lcd1602_write_4bitmode(struct i2c_lcd1602 *i2c_lcd1602, uint8_t data,
	uint8_t mode) {
	/* {{{ */
	uint8_t highnib = (data & 0xf0) | mode;
	uint8_t lownib = ((data << 4) & 0xf0) | mode;

	i2c_lcd1602_write_4bits(i2c_lcd1602, highnib);
	i2c_lcd1602_write_4bits(i2c_lcd1602, lownib);
	/* }}} */
}


uint8_t set_mode(uint8_t rs, uint8_t rw) {
	/* {{{ */
	uint8_t mode = 0x00; // 00000000

	if (rs == 1) mode |= Rs;
	if (rw == 1) mode |= Rw;

	return mode;
	/* }}} */
}
