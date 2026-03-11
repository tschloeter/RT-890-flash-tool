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

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include "common.h"
#include "radtel-serial.h"

static double elapsed(struct timespec *start, struct timespec *end)
{
	double const nano = 1e-9;
	long const giga = 1000000000L;
	
	long sec = end->tv_sec - start->tv_sec;
	long nsec = end->tv_nsec - start->tv_nsec;
	if (nsec < 0)
	{
		sec -= 1;
		nsec += giga;
	}
	return (double) sec + (double) nsec * nano;
}

uint8_t checksum(uint8_t const *data, ssize_t size)
{
	uint8_t sum = 0;

	while (size--)
	{
		sum += *data++;
	}

	return sum;
}

int check_communication_to_device(int fd_serial)
{
	uint8_t read_message[] = { RADTEL_CMD_READ_BLOCK, 0x00, 0x00, RADTEL_CMD_READ_BLOCK };
	uint8_t retry_count = 3;
	uint8_t response = 0;	

	serial_flush(fd_serial);

	do
	{
		if (sizeof(read_message) != serial_send(fd_serial, read_message, sizeof(read_message)))
		{
			fprintf(stderr, "Error transmitting READ_BLOCK command to device.\n");
			return RADTEL_MODE_UNKNOWN;
		}

		if (1 != serial_receive(fd_serial, &response, 1, 2.0))
		{
			fprintf(stderr, "Error receiving response from device.\n");
			return RADTEL_MODE_UNKNOWN;
		}

		serial_flush(fd_serial);

		switch (response)
		{
		case RADTEL_CMD_READ_BLOCK:
			return RADTEL_MODE_OPERATIONAL;
		case RADTEL_RECV_NOK:
			return RADTEL_MODE_BOOTLOADER;
		default:
		}
	} while (--retry_count);

	fprintf(stderr, "Too many retries while checking communication to device.\n");
	return RADTEL_MODE_UNKNOWN;
}

int serial_set_flags(int fd, speed_t baud)
{
	struct termios tty;

	if (E_SUCCESS != tcgetattr(fd, &tty))
	{
		fprintf(stderr, "Error getting serial port mode: tcgetattr() failed.\n");
		return E_FAILURE;
	}

	cfsetospeed(&tty, baud);
	cfsetispeed(&tty, baud);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;		// 8 bits
	tty.c_iflag &= ~IGNBRK;
	tty.c_lflag = 0;								// raw mode
	tty.c_oflag = 0;
	tty.c_cc[VMIN] = 0;								// synchronous wait with timeout
	tty.c_cc[VTIME] = 5;							// 0.5 s timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY);			// no SW flow control
	tty.c_cflag |= (CLOCAL | CREAD);				// receiver enable
	tty.c_cflag &= ~(PARENB | PARODD);				// no parity
	tty.c_cflag &= ~CSTOPB;							// 1 stop bit
	tty.c_cflag &= ~CRTSCTS;						// no HW flow control

	if (E_SUCCESS != tcsetattr(fd, TCSANOW, &tty))
	{
		fprintf(stderr, "Error setting serial port mode: txsetattr() failed.\n");
		return E_FAILURE;
	}
	
	return E_SUCCESS;
}

int serial_open(char const *device, speed_t speed)
{
	int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
	if (0 > fd)
	{
		fprintf(stderr, "Error opening serial port %s: %s\n", device, strerror(errno));
		return E_FAILURE;
	}

	if (E_SUCCESS != serial_set_flags(fd, speed))
	{
		close(fd);
		return E_FAILURE;
	}

	return fd;
}

void serial_flush(int fd)
{
	uint8_t dummy_buffer;
	while(0 < read(fd, &dummy_buffer, 1));
}

void serial_close(int fd)
{
	if (0 > close(fd))
	{
		fprintf(stderr, "Error closing serial port.\n");
	}
}

ssize_t serial_receive(int fd, void *buf, ssize_t size, double timeout_sec)
{
	ssize_t total = 0;
	struct timespec start;

	clock_gettime(CLOCK_MONOTONIC, &start);

	while (total < size)
	{
		ssize_t bytes_received = read(fd, (uint8_t *) buf + total, size - total);
		if (0 < bytes_received)
		{
			total += bytes_received;
		}
		else if (0 > bytes_received)
		{
			if (EINTR == errno)
			{
				continue;
			}
			fprintf(stderr, "Error reading from serial port: %s\n", strerror(errno));
			break;
		}

		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		if (timeout_sec <= elapsed(&start, &now))
		{
			fprintf(stderr, "Timeout waiting for %zu bytes from serial port.\n", size);
			break;
		}
	}

	return total;
}

ssize_t serial_send(int fd, void *buf, ssize_t size)
{
	ssize_t total = 0;

	while (total < size)
	{
		ssize_t bytes_sent = write(fd, (uint8_t *) buf + total, size - total);
		if (0 >= bytes_sent)
		{
			if (EINTR == errno)
			{
				continue;
			}
			fprintf(stderr, "Error writing to serial port: %s\n", strerror(errno));
			break;
		}
		total += bytes_sent;
	}

	return total;
}