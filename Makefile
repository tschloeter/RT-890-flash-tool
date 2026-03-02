# 
# Radtel RT-890 Firmware & SPI Flash Tool
# 
# Copyright (c) Thomas Schloeter
# 
# This file is part of the Radtel RT-890 Firmware & SPI Flash Tool and is licensed under the MIT License.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 
# This software is provided as-is, without any warranty. Use at your own risk.
# 

# Simple Makefile for RT-890 Flasher Project

CC = gcc
CFLAGS = -Wall -Wextra -O2

# Source files
SRCS = file-ops.c firmware-flash.c radtel-flash.c radtel-serial.c spi-flash.c

# Header files
HEADERS = file-ops.h firmware-flash.h radtel-flash.h radtel-serial.h spi-flash.h

# Platform Independence
ifeq ($(OS),Windows_NT)
    TARGET = rt890-flash-tool.exe
    RM = rm -f
    RMEXT = *.o $(TARGET)
else
    TARGET = rt890-flash-tool
    RM = rm -f
    RMEXT = *.o $(TARGET)
endif

# Object files
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(RMEXT)

.PHONY: all clean
