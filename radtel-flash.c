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

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "common.h"
#include "radtel-serial.h"
#include "firmware-flash.h"
#include "spi-flash.h"
#include "file-ops.h"

static void print_usage(char const *prog)
{
	fprintf(stderr,
		"Usage: %s -p <port> [-b {s|f}] [-e] [-s <filename>] [-r <filename> [-f]] [-w <filename> [-v]]\n"
		"  -p <port>      Serial port (e.g. /dev/ttyUSB0)\n"
		"  -b {s|f}       Baudrate (s = 19200 or f = 115200)\n"
		"  -e             Erase MCU firmware\n"
		"  -s <filename>  Write system firmware from file to device\n"
		"  -r <filename>  Read SPI flash data from device to file\n"
		"  -w <filename>  Write SPI flash data from file to device\n"
		"  -v             Verify SPI flash data after programming\n"
		"  -f             Force creating output file if it exists\n"
		"\n\n",
		prog);
}

int main(int argc, char ** argv)
{
	bool force = false;
	bool erase = false;
	char *port = NULL;
	char *sflash_output_image = NULL;
	char *sflash_input_image = NULL;
	char *firmware_image = NULL;
	uint8_t device_mode = RADTEL_MODE_UNKNOWN;
	bool verify_after_write = false;
	speed_t baud = B115200;
	
	printf("Radtel RT-890 Firmware + SPI Flash Utility\n");
	printf("Copyright (c) 2026 by Thomas Schloeter\n\n");
	printf("This software is provided as-is, without any warranties. Use at your own risk.\n\n");

	char opt;
	while (-1 != (opt = getopt(argc, argv, "p:b:r:w:s:efv")))
	{
		switch (opt)
		{
		case 'p':
			port = optarg;
			printf("Using serial port: %s\n", port);
			break;

		case 'b':
			switch (optarg[0])
			{
			case 's':
				printf("Selecting baudrate 19200 for serial connection\n");
				baud = B19200;
				break;

			case 'f':
				printf("Selecting baudrate 115200 for serial connection\n");
				baud = B115200;
				break;

			default:
				printf("Unsupported baudrate. Defaulting to 115200\n");
				baud = B115200;
			}
			break;

		case 'e':
			erase = true;
			printf("Mode: Erase Flash Memory\n");
			break;

		case 'r':
			sflash_output_image = optarg;
			printf("Mode: Copy SPI Flash to image file %s\n", sflash_output_image);
			break;

		case 'w':
			sflash_input_image = optarg;
			printf("Mode: Program SPI Flash from image file %s\n", sflash_input_image);
			break;

		case 's':
			erase = true;
			firmware_image = optarg;
			printf("Mode: Program System Firmware from image file %s\n", firmware_image);
			break;

		case 'f':
			force = true;
			break;

		case 'v':
			verify_after_write = true;
			break;

		default:
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (NULL == port)
	{
		fprintf(stderr, "Error: Serial port not specified\n");
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	printf("\nOpening serial port %s at baudrate %u\n", port, (baud == B19200) ? 19200 : 115200);

	int fd_serial = serial_open(port, baud);
	if (0 > fd_serial)
	{
		fprintf(stderr, "Error opening serial port %s\n", port);
		return EXIT_FAILURE;
	}

	bool repeat;
	printf("Serial port opened successfully. Checking communication with device...\n");
	do
	{
		repeat = false;
		device_mode = check_communication_to_device(fd_serial);
		switch(device_mode)
		{
			case RADTEL_MODE_BOOTLOADER:
				printf("Radio is in bootloader mode.\n");
				if (true == erase)
				{
					printf("Erasing flash memory\n");
					if (E_SUCCESS != erase_firmware(fd_serial))
					{
						fprintf(stderr, "Failed to erase flash memory.\n");
						serial_close(fd_serial);
						return EXIT_FAILURE;
					}
					else
					{
						erase = false;
					}
				}

				if (NULL != firmware_image)
				{
					printf("Programming system firmware from file %s...\n", firmware_image);
					if (E_SUCCESS != flash_firmware(fd_serial, firmware_image))
					{
						fprintf(stderr, "Failed to program system firmware.\n");
						serial_close(fd_serial);
						return EXIT_FAILURE;
					}
					else if ((NULL != sflash_input_image) || (NULL != sflash_output_image))
					{ 

						firmware_image = NULL;
						printf("Waiting %u seconds for the new firmware to start up...\n", REBOOT_DELAY);
						sleep(REBOOT_DELAY);
						repeat = true;
					}
				}
				else if ((NULL != sflash_input_image) || (NULL != sflash_output_image))
				{
					fprintf(stderr, "Device needs to be in operational mode for programming or dumping SPI flash.\n");
					serial_close(fd_serial);
					return EXIT_FAILURE;
				}
				break;

			case RADTEL_MODE_OPERATIONAL:
				printf("Radio is in operational mode.\n");
				if ((true == erase) || (NULL != firmware_image))
				{
					fprintf(stderr, "Device needs to be in bootloader mode for programming system firmware.\n");
					serial_close(fd_serial);
					return EXIT_FAILURE;
				}

				if (NULL != sflash_output_image)
				{
					printf("Dumping SPI flash to file %s...\n", sflash_output_image);
					if (E_SUCCESS != dump_spi_memory(fd_serial, sflash_output_image, force))
					{
						fprintf(stderr, "Failed to dump SPI flash to file.\n");
						serial_close(fd_serial);
						return EXIT_FAILURE;
					}
					printf("SPI flash successfully dumped to file.\n");
				}

				if (NULL != sflash_input_image)
				{
					printf("Programming SPI Flash from file %s...\n", sflash_input_image);

					if (E_SUCCESS != flash_spi_memory(fd_serial, sflash_input_image, verify_after_write))
					{
						fprintf(stderr, "Failed to program SPI flash image to device.\n");
						serial_close(fd_serial);
						return EXIT_FAILURE;
					}
					printf("SPI Flash programmed successfully.\n");
				}
				break;

			default:
				fprintf(stderr, "Communication to device could not be established.\n");
				serial_close(fd_serial);
				return EXIT_FAILURE;
		}
	} while (true == repeat);
	
	printf("All operations completed successfully. Closing serial port.\n");

	serial_close(fd_serial);
	return EXIT_SUCCESS;
}
