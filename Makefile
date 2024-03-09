# Makefile
CFLAGS = -Wall
CC = gcc


# Create object file for library
i2c-LCD1602.o: i2c-LCD1602.c i2c-LCD1602.h
	$(CC) $(CFLAGS) i2c-LCD1602.c -c -o i2c-LCD1602.o
