# Makefile
INCS = -I..
CFLAGS = -Wall
CC = gcc


# Create the example executable
i2c-lcd-test: i2c-lcd-test.c i2c-lcd-page-wrapper.o ../i2c-LCD1602.o
	$(CC) $(CFLAGS) $(INCS) i2c-lcd-test.c i2c-lcd-page-wrapper.o ../i2c-LCD1602.o -o i2c-lcd-test

i2c-lcd-page-wrapper.o: i2c-lcd-page-wrapper.c i2c-lcd-page-wrapper.h
	$(CC) $(CFLAGS) $(INCS) i2c-lcd-page-wrapper.c -c -o i2c-lcd-page-wrapper.o

# Overwrite default rule of compiling object files as we will rely on
# the library compiling its own object file
%.o: %.c
	@echo; \
	echo "ERROR: You may need to run 'make' in parent directory to compile \
	the object file for the i2c-LCD1602 library first"; \
	echo
