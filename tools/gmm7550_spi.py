#!/usr/bin/env python3
#
# This file is a part of the GMM-7550/RP2040 Control library
# <https://github.com/gmm-7550/gmm7550-control-rp2040.git>
#
# SPDX-License-Identifier: MIT
#
# Copyright (c) 2022-2026 Anton Kuzmin <ak@gmm7550.dev>

'''Command line tool to configure FPGA and program
NOR Flash on the GMM-7550 module via SPI on the RP2040
USB adapter board'''

__version__ = '0.7.0'

import sys
import time
import argparse
from serial import Serial

SERIAL_SPI_BLOCK_SIZE = 64
SERIAL_SPI_DEFAULT_PORT = "/dev/ttyACM2"

def load_fpga_config(fname, port=SERIAL_SPI_DEFAULT_PORT):
    with open(fname, mode='br') as f:
        data = f.read()
        data_len = len(data)

    with Serial(port) as spi:
        for start in range(0, data_len, SERIAL_SPI_BLOCK_SIZE):
            block = data[start:start + SERIAL_SPI_BLOCK_SIZE]
            count = len(block)
            spi.write(block)
            spi.read(count)

if __name__ == '__main__':
    p = argparse.ArgumentParser(description = __doc__)
    p.add_argument('-v', '--verbose', action='store_true', help='be more verbose')

    p.add_argument('-i', '--info',
                   action='store_true',
                   dest='spi_info',
                   help='Print SPI NOR device info')

    p.add_argument('-n', '--dry-run',
                   action='store_false',
                   dest='spi_execute',
                   help='Print the commands that would be executed, but do not execute write and erase operations')

    p.add_argument('-c', '--configure',
                   action='store_true',
                   help='Configure FPGA')

    p.add_argument('-r', '--read',
                   action='store_true',
                   dest='spi_read',
                   help='Read SPI NOR page(s) to file')

    p.add_argument('-w', '--write',
                   action='store_true',
                   dest='spi_write',
                   help='Write data from file to SPI NOR starting from the given page number')

    p.add_argument('-e', '--erase',
                   action='store_true',
                   dest='spi_erase',
                   help='Erase SPI NOR page(s)')

    p.add_argument('-a', '--addr',
                   dest='spi_addr', metavar='{address|from,to}',
                   help='Use SPI NOR address or address range')

    p.add_argument('-p', '--page',
                   dest='spi_page', metavar='{page|from,to}',
                   help='Use SPI NOR page or range of pages')

    p.add_argument('-s', '--sector',
                   dest='spi_sector', metavar='{sector|from,to}',
                   help='Use SPI NOR sector or range of sectors')

    p.add_argument('-b', '--block32',
                   dest='spi_block32', metavar='{block|from,to}',
                   help='Use SPI NOR block (32KiB) or range of blocks')

    p.add_argument('-B', '--block64',
                   dest='spi_block64', metavar='{block|from,to}',
                   help='Use SPI NOR block (64KiB) or range of blocks')

    p.add_argument('-C', '--chip',
                   action='store_true', dest='spi_chip',
                   help='Use entire SPI NOR chip')

    p.add_argument('-M', '--mem', type=str, dest='spi_mem',
                   choices=['east', 'west', 'north'],
                   help='Access SPI NOR on the memory add-on board')

    p.add_argument('-P', '--port', type=str,
                   default=SERIAL_SPI_DEFAULT_PORT,
                   help='Serial-to-SPI device (default: '+SERIAL_SPI_DEFAULT_PORT+')')

    p.add_argument('file',
                   nargs='?',
                   help='Filename to read/write SPI NOR data')

    args = p.parse_args()

    if args.configure:
        load_fpga_config(args.file, args.port)
