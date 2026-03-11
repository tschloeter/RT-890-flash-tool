/*
 * Radtel RT-890 Firmware & SPI Flash Tool
 *
 * Copyright (c) Thomas Schloeter
 * 
 * This file is part of the Radtel RT-890 Firmware & SPI Flash Tool and is licensed under the MIT License.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This software is provided as-is, without any warranty. Use at your own risk.
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <termios.h>

enum radtel_mode_t
{
	RADTEL_MODE_UNKNOWN = 0xFF,
	RADTEL_MODE_BOOTLOADER = 0,
	RADTEL_MODE_OPERATIONAL = 1
};

uint8_t checksum(uint8_t const *data, ssize_t size);
int serial_open(char const *device, speed_t speed);
void serial_close(int fd);
void serial_flush(int fd);
int serial_set_flags(int fd, speed_t baud);
ssize_t serial_receive(int fd, void *buf, ssize_t size, double timeout_sec);
ssize_t serial_send(int fd, void *buf, ssize_t size);
int check_communication_to_device(int fd_serial);

#endif