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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "common.h"
#include "radtel-serial.h"
#include "firmware-flash.h"
#include "file-ops.h"

int flash_firmware(int fd_serial, char const *firmware_file)
{
	uint8_t *flash_buffer = (uint8_t *) file_read(firmware_file, FIRMWARE_SIZE);

	if (NULL != flash_buffer)
	{
		if (E_SUCCESS != write_firmware_image(fd_serial, flash_buffer))
		{
			fprintf(stderr, "Error programming firmware image to device.\n");
			free(flash_buffer);
			serial_close(fd_serial);
			return E_FAILURE;
		}
		else
		{
			printf("System Firmware programmed successfully.\n");
			free(flash_buffer);
		}
	}
	else
	{
		fprintf(stderr, "Error reading firmware image from file.\n");
		free(flash_buffer);
		serial_close(fd_serial);
		return E_FAILURE;
	}

	return E_SUCCESS;
}

int write_firmware_image(int fd, void const *buffer)
{
	printf("Sending firmware blocks to device:\n");
	for (uint16_t address = 0; address < FIRMWARE_SIZE; address += BLOCK_SIZE)
	{
		printf("0x%04X ", address); fflush(stdout);
		if (E_SUCCESS != write_firmware_block(fd, address, buffer))
		{
			fprintf(stderr, "Error writing firmware flash page.\n");
			return E_FAILURE;
		}
		if (0 == ((address / BLOCK_SIZE) + 1) % PRINT_ENTRIES_PER_LINE)
		{
			printf("\n");
		}
	}
	printf("\n");

	return E_SUCCESS;
}

int write_firmware_block(int fd, uint16_t addr, void const *buffer)
{
	struct flash_block_t message;
	uint8_t *data = (uint8_t *) buffer + addr;

	message.cmd = RADTEL_CMD_WRITE_BLOCK;
	message.addr_hi = (uint8_t) (addr >> 8);
	message.addr_lo = (uint8_t) addr;
	memcpy(message.data, data, BLOCK_SIZE);
	message.checksum = checksum((uint8_t *) &message, sizeof(struct flash_block_t) - 1);

	if (sizeof(struct flash_block_t) != serial_send(fd, &message, sizeof(struct flash_block_t)))
	{
		fprintf(stderr, "\nError transmitting firmware block to device.\n");
		return E_FAILURE;
	}
		
	uint8_t response;
	if (1 != serial_receive(fd, &response, 1, 2.0))
	{
		fprintf(stderr, "\nError receiving firmware block response from device.\n");
		return E_FAILURE;
	}

	if (RADTEL_RECV_OK != response)
	{
		fprintf(stderr, "\nError flashing firmware block: Device returned negative response.\n");
		return E_FAILURE;
	}

	return E_SUCCESS;
}

int erase_firmware(int fd_serial)
{
	// This message has been taken from a serial capture of the manufacturer's original flasher.
	// Presumably this checks whether the radio is in the correct state (Boot / Application) or
	// prepares some kind of programming session.
	uint8_t message[] = { RADTEL_CMD_ERASE_FLASH, 0x33, 0x05, 0x10, 0x00 };
	uint8_t response = 0;	

	message[4] = checksum(message, 4);
	if (sizeof(message) != serial_send(fd_serial, message, sizeof(message)))
	{
		fprintf(stderr, "Error transmitting ERASE_FLASH command to device.\n");
		return E_FAILURE;
	}

	if (1 != serial_receive(fd_serial, &response, 1, 2.0))
	{
		fprintf(stderr, "Error receiving response from device.\n");
		return E_FAILURE;
	}

	if (RADTEL_RECV_OK != response)
	{
		fprintf(stderr, "Negative response from device; probably not in BOOT mode.\n");
		return E_FAILURE;
	}

	serial_flush(fd_serial);

	printf("Executing Erase Flash Memory Subcommand 0x55...\n");

	// Altering bytes 3 (subfunction?) to 0x55 lets the radio erase MCU's internal flash.
	message[3] = 0x55;
	message[4] = checksum(message, 4);
	if (sizeof(message) != serial_send(fd_serial, message, sizeof(message)))
	{
		fprintf(stderr, "Error transmitting ERASE_FLASH command to device.\n");
		return E_FAILURE;
	}

	if (1 != serial_receive(fd_serial, &response, 1, 2.0))
	{
		fprintf(stderr, "Error receiving response from device.\n");
		return E_FAILURE;
	}

	if (RADTEL_RECV_OK != response)
	{
		fprintf(stderr, "Negative response from device: flash not erased.\n");
		return E_FAILURE;
	}

	printf("Erasing flash memory completed.\n");

	return E_SUCCESS;
}
