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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

#define BLOCK_SIZE								(128u)
#define REBOOT_DELAY							(10u)
#define E_SUCCESS								(0)
#define E_FAILURE								(-1)
#define RADTEL_RECV_OK							(0x06)
#define RADTEL_RECV_NOK							(0xFF)
#define PRINT_ENTRIES_PER_LINE					(16)

enum radtel_cmd_t
{
	RADTEL_CMD_NOP					= 0x32u,
	RADTEL_CMD_ERASE_FLASH			= 0x39u,
	RADTEL_CMD_READ_BLOCK			= 0x52u, // Parameters: 128 byte-Block Address (16 bit), Checksum
	RADTEL_CMD_WRITE_BLOCK			= 0x57u, // Parameters: 128 byte-Block Address (16 bit), 128 bytes, Checksum
};

struct flash_block_t
{
	uint8_t cmd;
	uint8_t addr_hi;
	uint8_t addr_lo;
	uint8_t data[BLOCK_SIZE];
	uint8_t checksum;
};

#endif