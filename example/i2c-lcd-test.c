#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "i2c-LCD1602.h"
#include "i2c-lcd-page-wrapper.h"


int terminal_interface_init(struct termios *old_settings) {
	/* {{{ */
	struct termios terminal_interface;

	/* Save the old terminal interface settings */
	if (0 != tcgetattr(STDIN_FILENO, old_settings)) {
		fprintf(stderr, "Failed to get term attributes\n");
		return -1;
	}

	/* Get the settings for the terminal interface for modification */
	if (0 != tcgetattr(STDIN_FILENO, &terminal_interface)) {
		fprintf(stderr, "Failed to get term attributes\n");
		return -1;
	}

	/* Disable canonical mode (i.e. when input is only available line by line */
	terminal_interface.c_lflag &= ~ICANON;
	/* Disable local echoing */
	terminal_interface.c_lflag &= ~ECHO;

	/* Set VMIN and VTIME so that calling read() on this interface will block
	 * (time == 0) until at least 1 byte (min == 1) is available */
	terminal_interface.c_cc[VMIN] = 1;
	terminal_interface.c_cc[VTIME] = 0;

	/* Apply the settings for the terminal interface */
	if (0 != tcsetattr(STDIN_FILENO, TCSANOW, &terminal_interface)) {
		fprintf(stderr, "Failed to set term attributes\n");
		return -1;
	}

	return 0;
	/* }}} */
}


int terminal_interface_restore(struct termios *settings) {
	/* Apply the settings for the terminal interface */
	if (0 != tcsetattr(STDIN_FILENO, TCSADRAIN, settings)) {
		fprintf(stderr, "Failed to set term attributes\n");
		return -1;
	}

	return 0;
}


int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Invalid Number of Arguments...\n");
		printf("Usage: ./i2c-lcd-test <path-to-i2c-bus> <i2c-peripheral-address-in-hex>\n");
		printf(" e.g.: ./i2c-lcd-test /dev/i2c-1 0x27\n");
		return -1;
	}

	/* Parse the i2c lcd device path from the commandline args */
	char i2c_bus_path[512];
	strncpy(i2c_bus_path, argv[1], sizeof(i2c_bus_path));
	/* Parse the i2c peripheral address from the commandline args */
	int i2c_peripheral_addr;
	sscanf(argv[2], "%x", &i2c_peripheral_addr);

	int i2c_lcd_fd;

	/* If opening the i2c lcd device failed */
	if ( (i2c_lcd_fd = open(i2c_bus_path, O_RDWR)) < 0) {
		fprintf(stderr, "Failed to open the i2c lcd device\n");
		return -1;
	}

	/* Set the peripheral address for the controller */
	if (0 > ioctl(i2c_lcd_fd, I2C_SLAVE, i2c_peripheral_addr)) {
		fprintf(stderr, "Failed to set the peripheral address for the i2c controller\n");
		return -1;
	}

	/* Specify the qualities of our LCD:
	 *  - the FD
	 *  - the peripheral address
	 *  - the number of columns (16),
	 *  - the number of rows (2)
	 *  - the dotize (-1 atm because dotsize is unused),
	 *  - the backlight setting (backlight on). */
	struct i2c_lcd1602 i2c_lcd1602 = \
		i2c_lcd1602_init(i2c_lcd_fd, i2c_peripheral_addr, 16, 2, -1, \
		LCD_BACKLIGHT);
	struct i2c_lcd_page i2c_lcd = i2c_lcd_page_init(i2c_lcd1602);
	/* Perform the necessary startup instructions for our LCD. */
	i2c_lcd1602_begin(&i2c_lcd.i2c_lcd1602);

	int user_input_len;
	char user_input[64];
	/* A buffer of 4 characters to store the most recent 4 characters. The 0th
	 * element is the most recent */
	char recent_chars[4] = { 0, 0, 0, 0 };
	struct termios stdin_settings;

	if (0 != terminal_interface_init(&stdin_settings)) {
		fprintf(stderr, "Failed to initialize stdin as a terminal interface\n");
		return -1;
	}

	while (1) {
		/* Read a message from stdin */
		if (-1 == (user_input_len = read(STDIN_FILENO, user_input, sizeof(user_input)))) {
			fprintf(stderr, "Failed to read user input!\n");
		} else {
			/* Send the input, char by char, to the i2c device to get it to
			 * write the text */
			for (int i = 0; i < user_input_len; i++) {

				/* Update the recent chars buffer */
				for (int j = 3; j > 0; j--) {
					recent_chars[j] = recent_chars[j - 1];
				}
				recent_chars[0] = user_input[i];

				/* If the last few received characters represent an escape
				 * sequence, employ special case handling */
				if (recent_chars[0] == 27) {
					continue;
				} else if (recent_chars[1] == 27) {
					continue;
				} else if (recent_chars[2] == 27 && recent_chars[1] == 91) {
					/* If the user pressed the up arrow key */
					if (recent_chars[0] == 65) {
						/* If the cursor is not already on the first row ... */
						if (i2c_lcd.cursor_row > 0) {
							i2c_lcd_page_set_cursor_pos(&i2c_lcd, i2c_lcd.cursor_col, i2c_lcd.cursor_row - 1);
						}
					}
					/* If the user pressed the down arrow key */
					if (recent_chars[0] == 66) {
						/* If the cursor is not already on the last row ... */
						if (i2c_lcd.cursor_row < i2c_lcd.i2c_lcd1602.rows - 1) {
							i2c_lcd_page_set_cursor_pos(&i2c_lcd, i2c_lcd.cursor_col, i2c_lcd.cursor_row + 1);
						}
					}
					/* If the user pressed the right arrow key */
					if (recent_chars[0] == 67) {
						/* If the cursor is at the very right edge of the LCD,
						 * and there is still more of the row that can be
						 * displayed ... */
						if (i2c_lcd.cursor_col  >= i2c_lcd.i2c_lcd1602.columns - 1 \
							&& i2c_lcd.display_pos < (i2c_lcd.row_width - i2c_lcd.i2c_lcd1602.columns)) {
							/* ... shift the display left and the cursor right */
							i2c_lcd_page_shift(&i2c_lcd, 1, 0);
							i2c_lcd_page_shift(&i2c_lcd, 0, 1);
						/* If the cursor is NOT at the very right edge of the
						 * LCD ... */
						} else if (i2c_lcd.cursor_col < i2c_lcd.i2c_lcd1602.columns - 1) {
							/* ... otherwise shift just the cursor right */
							i2c_lcd_page_shift(&i2c_lcd, 0, 1);
						}
					}
					/* If the user pressed the left arrow key */
					if (recent_chars[0] == 68) {
						/* If the cursor is at the very left edge of the LCD,
						 * and there are characters we can shift left for ... */
						if (i2c_lcd.cursor_col <= 0 && i2c_lcd.display_pos > 0) {
							/* ... shift the display right and the cursor left */
							i2c_lcd_page_shift(&i2c_lcd, 1, 1);
							i2c_lcd_page_shift(&i2c_lcd, 0, 0);
						/* If the cursor is NOT at the very left edge of the
						 * LCD ... */
						} else if (i2c_lcd.cursor_col  > 0) {
							/* ... just shift the cursor left */
							i2c_lcd_page_shift(&i2c_lcd, 0, 0);
						}
					}

					continue;
				}
				/* If the input character is a Ctrl+l */
				if (user_input[i] == 12) {
					/* Turn off the backlight if the backlight is on, and
					 * turn on the backlight if it is off */
					if (i2c_lcd.i2c_lcd1602.backlight == LCD_BACKLIGHT) {
						i2c_lcd1602_set_backlight(&i2c_lcd.i2c_lcd1602, LCD_NOBACKLIGHT);
					} else if (i2c_lcd.i2c_lcd1602.backlight == LCD_NOBACKLIGHT) {
						i2c_lcd1602_set_backlight(&i2c_lcd.i2c_lcd1602, LCD_BACKLIGHT);
					}

					continue;
				}

				/* If the input character is a backspace */
				if (user_input[i] == 127) {
					/* Depending on which direction the cursor moves
					 * after receiving a character, either backspace
					 * to the left, or backspace to the right */
					if (i2c_lcd.i2c_lcd1602.entry_shift_increment == LCD_ENTRYINCREMENT) {
						/* If the cursor is at the left edge of the LCD,
						 * and there are characters we can shift left for ... */
						/* + 1 so we always see at least 1 character in the
						 * direction we are backspacing */
						if (i2c_lcd.cursor_col <= 0 + 1 && i2c_lcd.display_pos > 0) {
							/* Shift the display right */
							i2c_lcd_page_shift(&i2c_lcd, 1, 1);
							/* Shift the cursor left */
							i2c_lcd_page_shift(&i2c_lcd, 0, 0);
							/* Write a space */
							i2c_lcd_page_send_char(&i2c_lcd, ' ');
							/* Shift the cursor left */
							i2c_lcd_page_shift(&i2c_lcd, 0, 0);
						/* If the cursor is NOT at the left edge of the LCD ... */
						} else if (i2c_lcd.cursor_col  > 0) {
							/* Shift the cursor left */
							i2c_lcd_page_shift(&i2c_lcd, 0, 0);
							/* Write a space */
							i2c_lcd_page_send_char(&i2c_lcd, ' ');
							/* Shift the cursor left */
							i2c_lcd_page_shift(&i2c_lcd, 0, 0);
						}
					} else if (i2c_lcd.i2c_lcd1602.entry_shift_increment == LCD_ENTRYDECREMENT) {
						/* If the cursor is at the right edge of the LCD,
						 * and there are characters we can shift left for ... */
						/* - 1 so we always see at least 1 character in the
						 * direction we are backspacing */
						if (i2c_lcd.cursor_col >= (i2c_lcd.i2c_lcd1602.columns - 1) - 1 \
							&& i2c_lcd.display_pos + i2c_lcd.cursor_col < i2c_lcd.row_width) {

							/* Shift the display left */
							i2c_lcd_page_shift(&i2c_lcd, 1, 0);
							/* Shift the cursor right */
							i2c_lcd_page_shift(&i2c_lcd, 0, 1);
							/* Write a space */
							i2c_lcd_page_send_char(&i2c_lcd, ' ');
							/* Shift the cursor right */
							i2c_lcd_page_shift(&i2c_lcd, 0, 1);
						/* If the cursor is NOT at the right edge of the LCD ... */
						} else if (i2c_lcd.cursor_col  > 0) {
							/* Shift the cursor right */
							i2c_lcd_page_shift(&i2c_lcd, 0, 1);
							/* Write a space */
							i2c_lcd_page_send_char(&i2c_lcd, ' ');
							/* Shift the cursor right */
							i2c_lcd_page_shift(&i2c_lcd, 0, 1);
						}
					}

					continue;
				}

				/* If the input character is between ' ' and '~', the
				 * characters where the LCD character code matches ASCII. In
				 * other words, filter the input to printable characters only
				 * */
				if (user_input[i] >= 32 && user_input[i] <= 126) {
					/* If sending the char would go past the right boundary of
					 * a row ... */
					if (i2c_lcd.cursor_col + i2c_lcd.display_pos == i2c_lcd.row_width - 1 \
						&& i2c_lcd.i2c_lcd1602.entry_shift_increment == LCD_ENTRYINCREMENT) {

						/* ... shift the cursor left after sending the char */
						i2c_lcd_page_send_char(&i2c_lcd, user_input[i]);
						i2c_lcd_page_shift(&i2c_lcd, 0, 0);
					/* If sending the char would go past the left boundary of
					 * a row ... */
					} else if (i2c_lcd.cursor_col + i2c_lcd.display_pos == 0 \
						&& i2c_lcd.i2c_lcd1602.entry_shift_increment == LCD_ENTRYDECREMENT) {

						/* ... shift the cursor right after sending the char */
						i2c_lcd_page_send_char(&i2c_lcd, user_input[i]);
						i2c_lcd_page_shift(&i2c_lcd, 0, 1);
					} else {
						/* If the cursor is at the right edge of display window
						 * and the receiving the character would move the
						 * cursor to the right ... */
						if (i2c_lcd.cursor_col == i2c_lcd.i2c_lcd1602.columns - 1 \
							&& i2c_lcd.i2c_lcd1602.entry_shift_increment == LCD_ENTRYINCREMENT) {
							/* ... send the character and shift the display
							 * left */
							i2c_lcd_page_send_char(&i2c_lcd, user_input[i]);
							i2c_lcd_page_shift(&i2c_lcd, 1, 0);
						} else if (i2c_lcd.cursor_col == 0 \
							&& i2c_lcd.i2c_lcd1602.entry_shift_increment == LCD_ENTRYDECREMENT) {
							/* ... send the character and shift the display
							 * right */
							i2c_lcd_page_send_char(&i2c_lcd, user_input[i]);
							i2c_lcd_page_shift(&i2c_lcd, 1, 1);
						} else {
							i2c_lcd_page_send_char(&i2c_lcd, user_input[i]);
						}
					}

					continue;
				}
			}
		}
	}

	/* Never reached */
	if (0 != terminal_interface_restore(&stdin_settings)) {
		fprintf(stderr, "Failed to restore stdin\n");
		return -1;
	}
	close(i2c_lcd_fd);

	return 0;
}
