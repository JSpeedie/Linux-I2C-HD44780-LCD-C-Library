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
	 *  - the backlight setting (1 to have the backlight on). */
	struct i2c_lcd1602 i2c_lcd = \
		i2c_lcd1602_init(i2c_lcd_fd, i2c_peripheral_addr, 16, 2, -1, 1);
	/* Perform the necessary startup instructions for our LCD. */
	i2c_lcd1602_begin(&i2c_lcd);

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
						// TODO:
					}
					/* If the user pressed the down arrow key */
					if (recent_chars[0] == 66) {
						// TODO:
					}
					/* If the user pressed the right arrow key */
					if (recent_chars[0] == 67) {
						i2c_lcd1602_shift(&i2c_lcd, 0, 1);
					}
					/* If the user pressed the left arrow key */
					if (recent_chars[0] == 68) {
						i2c_lcd1602_shift(&i2c_lcd, 0, 0);
					}

					continue;
				}

				/* If the input character is a backspace */
				if (user_input[i] == 127) {
					// Move cursor back 1 spot
					// write a space
					// move the cursor back 2 spots?
					//
					// OR
					//
					// set entry mode to shift left
					// write space
					// set entry mode to shift right
					//
					i2c_lcd1602_shift(&i2c_lcd, 0, 0);
				}

				/* If the input character is between ' ' and '~', the
				 * characters where the LCD character code matches ASCII. In
				 * other words, filter the input to printable characters only
				 * */
				if (user_input[i] >= 32 && user_input[i] <= 126) {
					i2c_lcd1602_send_char(&i2c_lcd, user_input[i]);
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
