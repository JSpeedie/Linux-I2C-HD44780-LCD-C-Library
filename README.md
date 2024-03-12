## Setup

### Software setup

Before you can use the GPIO pins for i2c usage, we must first install
some packages and change the raspberry pi's config.

Run the following commands:

```bash
# To install the i2c tools you will need
sudo apt-get install i2c-tools
# To enable i2c support on your raspberry pi
sudo raspi-config
```

You will be met with a dialog with several options. You will want to select "3
Interface Options" and from there, "P5 I2C", selecting "\<Yes\>" for enabling
the ARM I2C interface. It will summarize the options you have set, to which you
can hit "Ok", and then you can navigate to "\<Finish\>" and leave the dialog.

A restart will likely be required.

### Hardware connections

Once you have the necessary software installed/configured, we need to
connect the hardware. Pictured below is the necessary setup:

<p align="center">
  <img src="https://raw.githubusercontent.com/wiki/JSpeedie/I2C-HD44780-LCD-C-Library/images/RaspberryPi-I2C-LCD1602.png" width="50%"/>
</p>


## Compiling

### The I2C LCD1602 Library

If you wish to make use of this library elsewhere, you can copy `i2c-LCD1602.c`
and `i2c-LCD1602.h` to your project and then just make sure you compile them
as a part of your project. There's nothing complicated about the compilation -
take a look at the `Makefile`.

### The Example Program

First, make sure you are at the root of your local copy of this git repo. Then
run:

```bash
make
```

This will generate `i2c-LCD1602.o`. Now you can run the following commands:

```bash
cd example/
make
```

This will produce the example program `i2c-lcd-test`.


### Running the Example Program

Once the necessary software has been installed/configured, the I2C LCD has been
connected to the raspberry pi correctly, and the example program executable has
been compiled we can check some information pertaining to the I2C LCD that we
will need in order to make use of it.

```bash
# Check what i2c device files are available to us
ls -l /dev | grep i2c
```

Which might generate output like:

```
crw-rw----  1 root i2c      89,   1 Mar  5 21:45 i2c-1
```

In this case, the "1" at the end of `i2c-1` is what we need to remember. Now
we can see what's on the i2c bus:

```bash
# The value given to the y flag needs to match the number that came after
# i2c-x in the previous step
i2cdetect -y 1
```
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- 27 -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- --
```

The output may look slightly different for you, but assuming you only have one
i2c device connected and it's the LCD, you can just take note of the address in
the output, namely `27` or as we will need to refer to it later: `0x27`.

Finally we can run the example program:

```bash
cd example/
./i2c-lcd-test /dev/i2c-1 0x27
```

When the program is running it will read input from `stdin` and either print it
on the LCD or have the LCD respond appropriately (if, say, the arrow keys or
backspace are pressed).
