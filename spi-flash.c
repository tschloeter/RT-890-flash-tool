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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "common.h"
#include "radtel-serial.h"
#include "file-ops.h"
#include "spi-flash.h"

int dump_spi_memory(int fd_serial, char const *sflash_output_image, bool force)
{
	uint8_t *flash_buffer = (uint8_t *) read_sflash_image(fd_serial);

	if (NULL == flash_buffer)
	{
		fprintf(stderr, "Unable to read flash image from device.\n");
		return E_FAILURE;
	}
	else
	{
		if (E_SUCCESS != file_save(sflash_output_image, flash_buffer, SFLASH_SIZE, force))
		{
			fprintf(stderr, "Error saving flash dump file.\n");
			free(flash_buffer);
			return E_FAILURE;
		}
		else
		{
			free(flash_buffer);
		}
	}

	return E_SUCCESS;
}

void *read_sflash_image(int fd)
{
	uint8_t *flash_buffer = malloc(SFLASH_SIZE);
	if (!flash_buffer)
	{
		fprintf(stderr, "Unable to allocate %u bytes of memory\n", SFLASH_SIZE);
		return NULL;
	}
	
	printf("Receiving flash pages from device:\n");
	uint8_t *read_position = flash_buffer;
	for (uint16_t page = 0; page < SFLASH_SIZE / SFLASH_PAGE_SIZE; page++)
	{
		printf("0x%04X ", page); fflush(stdout);
		if (E_SUCCESS != read_sflash_page(fd, page, read_position))
		{
			free(flash_buffer); fflush(stdout);
			return NULL;
		}
		if (0 == (page + 1) % PRINT_ENTRIES_PER_LINE)
		{
			printf("\n");
		}
		read_position += SFLASH_PAGE_SIZE;
	}
	printf("\n");
	
	return flash_buffer;
}

int read_sflash_page(int fd, uint16_t page, void *buf)
{
	uint16_t addr = page * (SFLASH_PAGE_SIZE / BLOCK_SIZE);
	uint8_t *data = (uint8_t *) buf;

	for (uint8_t block_num = 0; block_num < SFLASH_PAGE_SIZE / BLOCK_SIZE; block_num++)
	{
		uint8_t read_cmd[4];
		read_cmd[0] = RADTEL_CMD_READ_BLOCK;
		read_cmd[1] = (uint8_t) (addr >> 8);
		read_cmd[2] = (uint8_t) addr;
		read_cmd[3] = checksum(read_cmd, 3);

		if (4 != serial_send(fd, read_cmd, 4))
		{
			fprintf(stderr, "\nError transmitting serial command.\n");
			return E_FAILURE;
		}

		struct flash_block_t block;
		if (sizeof(struct flash_block_t) != serial_receive(fd, &block, sizeof(struct flash_block_t), 2.0))
		{
			fprintf(stderr, "\nError receiving flash block from device.\n");
			return E_FAILURE;
		}
		if ((RADTEL_CMD_READ_BLOCK != block.cmd)
			|| (block.addr_hi != read_cmd[1])
			|| (block.addr_lo != read_cmd[2])
			|| (block.checksum != checksum((uint8_t *) &block, sizeof(struct flash_block_t) - 1)))
		{
			fprintf(stderr, "\nError reading flash page. Malformed response from device.\n");
			return E_FAILURE;
		}
		memcpy(data, block.data, BLOCK_SIZE);
		data += BLOCK_SIZE;
		addr++;
	}
	
	return E_SUCCESS;
}

int flash_spi_memory(int fd_serial, char const *sflash_input_image, bool verify_after_write)
{
	uint8_t *flash_buffer = (uint8_t *) file_read(sflash_input_image, SFLASH_SIZE);
	
	if (NULL == flash_buffer)
	{
		fprintf(stderr, "Error reading flash image from file.\n");
		return E_FAILURE;
	}
	else
	{
		if (E_SUCCESS != write_sflash_image(fd_serial, flash_buffer, verify_after_write))
		{
			fprintf(stderr, "Error programming flash image to device.\n");
			free(flash_buffer);
			return E_FAILURE;
		}
		else
		{
			free(flash_buffer);
		}
	}

	return E_SUCCESS;
}

int write_sflash_image(int fd, uint8_t const *buffer, bool verify_after_write)
{
	uint8_t const *write_position = buffer;

	printf("Sending flash pages to device:\n");
	for (uint16_t page = 0; page < SFLASH_SIZE / SFLASH_PAGE_SIZE; page++)
	{
		printf("0x%04X", page); fflush(stdout);

		if (E_SUCCESS != write_sflash_page(fd, page, write_position))
		{
			return E_FAILURE;
		}
		if (verify_after_write)
		{
			printf(":W"); fflush(stdout);
			if (E_SUCCESS != verify_sflash_page(fd, page, write_position))
			{
				return E_FAILURE;
			}
			else
			{
				printf("V "); fflush(stdout);
			}
		}
		else
		{
			printf(" "); fflush(stdout);
		}
		if (0 == (page + 1) % PRINT_ENTRIES_PER_LINE)
		{
			printf("\n");
		}
		write_position += SFLASH_PAGE_SIZE;
	}
	printf("\n");

	return E_SUCCESS;
}

int write_sflash_page(int fd, uint16_t page, void const *buffer)
{
	struct flash_block_t message;
	uint8_t *data = (uint8_t *) buffer;

	message.cmd = RADTEL_CMD_WRITE_BLOCK;

	for (uint8_t block_num = 0; block_num < SFLASH_PAGE_SIZE / BLOCK_SIZE; block_num++)
	{
		uint16_t addr = (page * (SFLASH_PAGE_SIZE / BLOCK_SIZE)) + block_num;
		message.addr_hi = (uint8_t) (addr >> 8);
		message.addr_lo = (uint8_t) addr;
		memcpy(message.data, data, BLOCK_SIZE);
		message.checksum = checksum((uint8_t *) &message, sizeof(struct flash_block_t) - 1);

		if (sizeof(struct flash_block_t) != serial_send(fd, &message, sizeof(struct flash_block_t)))
		{
			fprintf(stderr, "\nError transmitting flash block to device.\n");
			return E_FAILURE;
		}
		
		uint8_t response;
		if (1 != serial_receive(fd, &response, 1, 2.0))
		{
			fprintf(stderr, "\nError receiving flash block response from device.\n");
			return E_FAILURE;
		}
		if (RADTEL_RECV_OK != response)
		{
			fprintf(stderr, "\nError writing flash page. Device returned negative response.\n");
			return E_FAILURE;
		}
		data += BLOCK_SIZE;
	}

	return E_SUCCESS;
}

int verify_sflash_page(int fd, uint16_t page, uint8_t const *buffer)
{
	uint8_t receive_buffer[SFLASH_PAGE_SIZE];
	
	if (E_SUCCESS != read_sflash_page(fd, page, receive_buffer))
	{
		fprintf(stderr, "\nError reading page from device for verification.\n");
		return E_FAILURE;
	}

	if (0 != memcmp(receive_buffer, buffer, SFLASH_PAGE_SIZE))
	{
		fprintf(stderr, "\nFlash page verification failed for page 0x%04X.\n", page);
		return E_PAGE_NOK;
	}

	return E_SUCCESS;
}
