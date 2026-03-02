
# Radtel RT-890 Firmware & SPI Flash Tool

An open-source tool for flashing and backing up the firmware and SPI flash data of the Radtel RT-890 radio.

## Features
- Read and backup SPI flash contents
- Program SPI flash with binary files
- Flash the system firmware
- Erase the flash memory
- Serial communication with selectable baud rates

## Requirements
- GCC (Linux or Windows with Cygwin)
- make
- Standard C library
- Access to a serial port (e.g. /dev/ttyUSB0)

## Build Instructions

```sh
make
```

This will create the executable `rt890-spi-flasher` (or `rt890-spi-flasher.exe` on Windows).

## Usage

```sh
./rt890-spi-flasher -p <port> [options]
```

### Main Options

- `-p <port>`      Serial port (e.g. /dev/ttyUSB0 or COM3)
- `-b {s|f}`       Baudrate: s = 19200, f = 115200 (default: 115200)
- `-e`             Erase MCU flash (firmware)
- `-s <file>`      Program system firmware from file
- `-r <file>`      Read SPI flash and save to file
- `-w <file>`      Program SPI flash from file
- `-v`             Verify SPI flash after writing
- `-f`             Overwrite output files without prompt

### Examples

**Read SPI flash:**

```sh
./rt890-spi-flasher -p /dev/ttyUSB0 -r backup.bin
```

**Program SPI flash and verify pages written:**

```sh
./rt890-spi-flasher -p /dev/ttyUSB0 -w image.bin -v
```

**Flash system firmware:**

```sh
./rt890-spi-flasher -p /dev/ttyUSB0 -s firmware.bin
```

**Erase flash:**

```sh
./rt890-spi-flasher -p /dev/ttyUSB0 -e
```

## License

MIT License — see file headers in the source code.

## Disclaimer

This tool is provided as-is, without any warranty. Use at your own risk!

