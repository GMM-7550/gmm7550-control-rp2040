#!/usr/bin/env python3

import sys
from serial import Serial

SPI_BLOCK_SIZE = 64

def main():
    with open(sys.argv[1], mode='br') as f:
        data = f.read()
        data_len = len(data)

    with Serial(port='/dev/ttyACM2') as spi:
        for start in range(0, data_len, SPI_BLOCK_SIZE):
            block = data[start:start + SPI_BLOCK_SIZE]
            count = len(block)
            spi.write(block)
            spi.read(count)

if __name__ == '__main__':
    main()
