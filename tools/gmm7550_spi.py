#!/usr/bin/env python3
#
# This file is a part of the GMM-7550/RP2040 Control library
# <https://github.com/gmm-7550/gmm7550-control-rp2040.git>
#
# SPDX-License-Identifier: MIT
#
# Copyright (c) 2022-2026 Anton Kuzmin <ak@gmm7550.dev>

'''Command line tool to configure FPGA and program NOR Flash on the
GMM-7550 module via SPI on the RP2040 USB adapter board.  Note that SPI
multiplexer and FPGA configuration mode should be set to the correct
values via CLI interface.
'''

__version__ = '0.7.1'

import os
import sys
import argparse
import logging
from serial import Serial
from functools import reduce

SERIAL_SPI_BLOCK_SIZE = 64
SERIAL_SPI_DEFAULT_PORT = "/dev/ttyACM2"

SERIAL_SPI_DEFAULT_BIT_RATE = 50
SERIAL_SPI_SLOW_BIT_RATE    = 15

logging.basicConfig(stream=sys.stderr, level=logging.WARNING)
log = logging.getLogger('gmm7550_spi')

def load_fpga_config(fname, port):
    with open(fname, mode='br') as f:
        data = f.read()
        data_len = len(data)

    with Serial(port) as spi:
        for start in range(0, data_len, SERIAL_SPI_BLOCK_SIZE):
            block = data[start:start + SERIAL_SPI_BLOCK_SIZE]
            count = len(block)
            spi.write(block)
            spi.read(count)

def get_spi_ids(port):
    spi_ids = []
    with Serial(port) as spi:
        # Slow-down SPI (SPI NOR may be connected through FPGA bridging)
        spi.baudrate = SERIAL_SPI_SLOW_BIT_RATE
        spi.write([0x9f, 0, 0, 0]) # Read JEDEC ID
        spi.flush()
        id = spi.read(4)
        spi_ids.append(id[1:])

        # Deactiate SPI /SS signal (to end RDJDID command)
        spi.rts = False
        # 3 address + 1 dummy + 16 bytes UID
        uid = [0 for i in range(3 + 1 + 16)]
        uid.insert(0, 0x4b) # RDUID command
        spi.rts = True      # Activate SPI /SS
        spi.write(uid)
        spi.flush()
        uid = spi.read(5 + 16) # 1+3+1 = 5
        spi.rts = False
        spi.baudrate = SERIAL_SPI_DEFAULT_BIT_RATE

        spi_ids.append(uid[5:])
        return spi_ids

def print_spi_id(ids):
    id = ids[0]

    print('SPI-NOR Flash JEDEC ID')
    print('  manufacturer: %02x' % id[0])
    print('   memory type: %02x' % id[1])
    print('      capacity: %02x' % id[2])

    uid = reduce(lambda s, b : s + " %02x" % b, ids[1],
                 '           UID:')
    print(uid)

def main():
    p = argparse.ArgumentParser(description = __doc__)

    p.add_argument('-V', '--version', action='version', version=__version__)

    p.add_argument('-v', '--verbose', action='count', default=0, help='be more verbose')

    p.add_argument('-i', '--info',
                   action='store_true',
                   dest='spi_info',
                   help='Print SPI NOR device info')

    p.add_argument('-n', '--dry-run',
                   action='store_false',
                   dest='spi_execute',
                   help='Do not execute write and erase operations')

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
                   help='Access SPI NOR on the Memory add-on board')

    p.add_argument('-P', '--port', type=str,
                   default=SERIAL_SPI_DEFAULT_PORT,
                   help='Serial-to-SPI device (default: '+SERIAL_SPI_DEFAULT_PORT+')')

    p.add_argument('file',
                   nargs='?',
                   help='Filename to read/write SPI NOR data and FPGA configuration')

    args = p.parse_args()

    if args.verbose == 0:
        log.setLevel(logging.WARNING)
    elif args.verbose == 1:
        log.setLevel(logging.INFO)
    else: # >= 2
        log.setLevel(logging.DEBUG)

    log.debug('Verbosity level: %d', args.verbose)
    log.debug('Serial-to-SPI device: %s', args.port)

    # Basic arguments sanity check

    if not (args.spi_info or args.spi_read or
            args.spi_write or args.spi_erase or
            args.configure):
        log.error('At least one action [info/read/write/erase/configure] should be specified')
        return 1

    if (args.spi_read or args.spi_write or args.configure) and (args.file is None):
        log.error('FPGA configure and SPI NOR read and write operations require file to be specified')
        return 1

    # FPGA configuration with explicit command and configuration file

    if args.configure:
        log.info('Configure FPGA from file: %s', args.file)
        load_fpga_config(args.file, args.port)

    # FPGA configuration with pre-defined SPI bridge configuration.
    # It should be done before access to the SPI-NOR on a Memory module
    if args.spi_mem:
        log.info('Pre-configure FPGA with SPI bridge: %s' % args.spi_mem)
        cfg_dir = os.path.dirname(sys.argv[0])
        cfg_dir = os.path.abspath(cfg_dir) + '/fpga_configs/'
        cfg = cfg_dir + 'spi_bridge_' + args.spi_mem + '.bit'
        load_fpga_config(cfg, args.port)

    # Always read IDs to know the SPI-NOR chips size
    log.info('Get SPI NOR IDs')
    spi_ids = get_spi_ids(args.port)

    if args.spi_info:
        print_spi_id(spi_ids)

    return 0

if __name__ == '__main__':
    sys.exit(main())
