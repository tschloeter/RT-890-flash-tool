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
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include "common.h"
#include "file-ops.h"

int file_save(char const *filename, void const *buf, ssize_t size, bool force)
{
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | (force ? 0 : O_EXCL), FILE_CREATE_UMASK);
	if (0 > fd)
	{
		if (EEXIST == errno)
		{
			fprintf(stderr, "File %s already exists\n", filename);
		}
		else
		{
			fprintf(stderr, "Unable to open file %s for writing\n", filename);
		}
		return E_FAILURE;
	}

	ssize_t total = 0;
	while (total < size) 
	{
		ssize_t written = write(fd, (uint8_t const*) buf + total, size - total);
		if (0 >= written)
		{
			if (EINTR == errno)
			{
				continue;
			}
			fprintf(stderr, "Error writing to file %s: %s\n", filename, strerror(errno));
			if (0 > close(fd))
			{
				fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
			}
			return E_FAILURE;
		}
		total += written;
	}

	if (0 > close(fd))
	{
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		return E_FAILURE;
	}

	return E_SUCCESS;
}

void *file_read(char const *filename, ssize_t size)
{
	int fd = open(filename, O_RDONLY);
	if (0 > fd)
	{
		fprintf(stderr, "Unable to open file %s for reading: %s\n", filename, strerror(errno));
		return NULL;
	}

	struct stat filestat;
	if (0 > fstat(fd, &filestat))
	{
		fprintf(stderr, "Unable to stat file: %s\n", strerror(errno));
		if (0 > close(fd))
		{
			fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		}
		return NULL;
	}

	ssize_t filesize = filestat.st_size;

	if (size != filesize)
	{
		fprintf(stderr, "File %s has wrong input file size (%zu instead of %zu)\n", filename, filesize, size);
		if (0 > close(fd))
		{
			fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		}
		return NULL;
	}

	void *buf = malloc(size);
	if (NULL == buf)
	{
		fprintf(stderr, "Unable to allocate %zu bytes of memory\n", size);
		if (0 > close(fd))
		{
			fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		}
		return NULL;
	}

	ssize_t total = 0;
	while (total < size)
	{
		ssize_t read_size = read(fd, (uint8_t*)buf + total, size - total);
		if (0 >= read_size)
		{
			if (EINTR == errno)
			{
				continue;
			}
			fprintf(stderr, "Error or unexpected end of file while reading file %s: %s\n", filename, strerror(errno));
			free(buf);
			if (0 > close(fd))
			{
				fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
			}
			return NULL;
		}
		else
		{
			total += read_size;
		}
	}

	if (0 > close(fd))
	{
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
	}

	return buf;
}