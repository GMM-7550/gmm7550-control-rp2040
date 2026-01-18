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

__version__ = '0.7.2'

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

######################################################################
# Load FPGA Configuration via SPI interface
# GMM-7550 Settings:
# > cfg 4
# > mux 1
######################################################################

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

######################################################################
# SPI-NOR Functions
######################################################################

def spi_get_ids(port):
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

def spi_nop(s):
    s.rst = True
    s.write([0x00]); s.flush(); s.read(1)
    s.rst = False

def spi_read_status(s):
    s.rts = True
    s.write([0x05, 0x00])
    s.flush()
    rd = s.read(2)
    s.rts = False
    log.debug('spi_read_status: 0x%02x' % rd[1])
    return rd[1]

def spi_wait_idle(s):
    while True:
        sr = spi_read_status(s)
        if (sr & 0x01) == 0x00:
            return

def spi_write_enable(s):
    s.rts = True
    s.write([0x06]); s.flush()
    s.rts = False
    s.read(1)
    sr = spi_read_status(s)
    if (sr & 0x02) == 0x00:
        log.error('Cannot enable SPI Write')
        return False
    else:
        log.debug('spi_write_enable')
    return True

def spi_write_disable(s):
    s.rts = True
    s.write([0x04]); s.flush()
    s.rts = False
    s.read()

def spi_chip_erase(s):
    if spi_write_enable(s):
        s.rts = True
        s.write([0x60]); s.flush()
        s.rts = False
        s.read(1)
        spi_wait_idle(s)
        spi_write_disable(s)

def spi_cmd_erase(s, cmd, addr):
    xfer = [ cmd,
             (addr >> 16) & 0xff,
             (addr >>  8) & 0xff,
              addr        & 0xff ]
    if spi_write_enable(s):
        s.rts = True
        s.write(xfer); s.flush()
        s.rts = False
        s.read(4)
        spi_wait_idle(s)
        spi_write_disable(s)

def spi_sector_erase(s, addr):
    spi_cmd_erase(s, 0x20, addr)

def spi_block32_erase(s, addr):
    spi_cmd_erase(s, 0x52, addr)

def spi_block64_erase(s, addr):
    spi_cmd_erase(s, 0xd8, addr)

def spi_write(s, addr, data):
    data.insert(0, addr & 0xff)
    addr >>= 8
    data.insert(0, addr & 0xff)
    addr >>= 8
    data.insert(0, addr & 0xff)
    data.insert(0, 0x02) # Page Program command

    l = len(data)
    spi_wait_idle(s)
    if spi_write_enable(s):
        s.rts = True
        s.write(data); s.flush()
        s.rts = False
        s.read(l)
        spi_wait_idle(s)
        spi_write_disable(s)

def spi_read(s, addr, count = SERIAL_SPI_BLOCK_SIZE-4):
    if count >= SERIAL_SPI_BLOCK_SIZE-4:
        count = SERIAL_SPI_BLOCK_SIZE-4
    xfer = [0x00 for i in range(count)]
    xfer.insert(0, addr & 0xff)
    addr >>= 8
    xfer.insert(0, addr & 0xff)
    addr >>= 8
    xfer.insert(0, addr & 0xff)
    xfer.insert(0, 0x03) # NORD -- Normal Read
    s.rts = True
    s.write(xfer); s.flush()
    s.rts = False
    data = s.read(len(xfer))
    return data[4:]

######################################################################
# MAIN application
######################################################################

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

    p.add_argument('-N', '--no-hardware',
                   action='store_true',
                   help='Do not access hardware (even for read, use mock ID)')

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

    if args.no_hardware:
        spi_execute = False
        log.debug('No hardware access')

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
        if args.no_hardware:
            log.warning('No hardware access, operation ignored')
        else:
            load_fpga_config(args.file, args.port)

    # FPGA configuration with pre-defined SPI bridge configuration.
    # It should be done before access to the SPI-NOR on a Memory module
    if args.spi_mem:
        log.info('Pre-configure FPGA with SPI bridge: %s' % args.spi_mem)
        cfg_dir = os.path.dirname(sys.argv[0])
        cfg_dir = os.path.abspath(cfg_dir) + '/fpga_configs/'
        cfg = cfg_dir + 'spi_bridge_' + args.spi_mem + '.bit'
        if args.no_hardware:
            log.warning('No hardware access, operation ignored')
        else:
            load_fpga_config(cfg, args.port)

    # Always read IDs to know the SPI-NOR chips size
    log.info('Get SPI NOR IDs')
    if args.no_hardware:
        log.warning('No hardware access, use mock IDs')
        spi_ids = [[0x9d, 0x60, 0x16],
                   [0x11 * i for i in range(16)]]
    else:
        spi_ids = spi_get_ids(args.port)

    if args.spi_info:
        print_spi_id(spi_ids)

    if spi_ids[0][0] != 0x9d : # ISSI
        log.error('Unknown Manufacturer ID: 0x%02x' % spi_ids[0][0])
        return 2

    if spi_ids[0][1] != 0x60 :
        log.error('Unknown Memory Type: 0x%02x' % spi_ids[0][1])
        return 2

    class spi_nor:
        page_size    =      256  # max programmable unit
        sector_size  =  4 * 1024 # minimal erase unit
        block32_size = 32 * 1024 # 32K block
        block64_size = 64 * 1024 # 64K blcok
        chip_size    = 0

    size_id = spi_ids[0][2]
    if size_id == 0x16: # configuration SPI-NOR on the GMM-7550 module
        log.info('IS25LP032 -- 32 Mbit (4 MiB)')
        spi_nor.chip_size = 4 * 1024 * 1024
    # elif size_id == 0x17:
    #     pass
    elif size_id == 0x18: # SPI-NOR on the Memory add-on board
        log.info('IS25LP128 -- 128 Mbit (16 MiB)')
        spi_nor.chip_size = 16 * 1024 * 1024
    else:
        log.error('Unknown Memory Size ID: 0x%02x' % size_id)
        return 2

    #
    # Process adddress arguments
    #
    from enum import Enum, auto
    class AddrUnit(Enum):
        undefined = auto()
        chip = auto()
        addr = auto()
        page = auto()
        sector = auto()
        block32 = auto()
        block64 = auto()

    def try_addr(arg):
        if arg is None:
            return False, 0, None
        else:
            _addrs = arg.split(',')
            if len(_addrs) == 1:
                return True, int(_addrs[0], 0), None
            else:
                return True, int(_addrs[0], 0), int(_addrs[1], 0)

    spi_start, spi_end, unit_size = 0, None, 1
    addr_unit = AddrUnit.undefined
    addr_error = False

    if args.spi_chip:
        addr_unit = AddrUnit.chip
        spi_start, spi_end = 0, spi_nor.chip_size-1
        unit_size = 1

    if not addr_error:
        _set, start, end = try_addr(args.spi_addr)
        addr_error = _set and addr_unit is not AddrUnit.undefined
        if _set and addr_unit is AddrUnit.undefined:
            addr_unit = AddrUnit.addr
            spi_start, spi_end = start, end
            unit_size = 1

    if not addr_error:
        _set, start, end = try_addr(args.spi_page)
        addr_error = _set and addr_unit is not AddrUnit.undefined
        if _set and addr_unit is AddrUnit.undefined:
            addr_unit = AddrUnit.page
            spi_start, spi_end = start, end
            unit_size = spi_nor.page_size

    if not addr_error:
        _set, start, end = try_addr(args.spi_sector)
        addr_error = _set and addr_unit is not AddrUnit.undefined
        if _set and addr_unit is AddrUnit.undefined:
            addr_unit = AddrUnit.sector
            spi_start, spi_end = start, end
            unit_size = spi_nor.sector_size

    if not addr_error:
        _set, start, end = try_addr(args.spi_block32)
        addr_error = _set and addr_unit is not AddrUnit.undefined
        if _set and addr_unit is AddrUnit.undefined:
            addr_unit = AddrUnit.block32
            spi_start, end = start, end
            unit_size = spi_nor.block32_size

    if not addr_error:
        _set, start, end = try_addr(args.spi_block64)
        addr_error = _set and addr_unit is not AddrUnit.undefined
        if _set and addr_unit is AddrUnit.undefined:
            addr_unit = AddrUnit.block64
            spi_start, spi_end = start, end
            unit_size = spi_nor.block64_size

    if addr_error:
        log.error('Address or address range should be specified only once, (-a, -p, -s, -b, -B, -C options are mutually exclusive)')
        return 1

    if addr_unit is AddrUnit.undefined and (args.spi_read or
                                            args.spi_write or
                                            args.spi_erase):
        log.error('Address should be specified for read, write, and erase operations with one of the options: -a, -p, -s, -b, -B, -C')
        return 1

    spi_start_addr = spi_start * unit_size

    if spi_end is None:
        spi_end_addr = spi_start_addr + unit_size - 1
    else:
        spi_end_addr = (spi_end + 1) * unit_size - 1

    log.info('SPI NOR address range for operation: 0x%06x..0x%06x' % (spi_start_addr, spi_end_addr))

    if spi_end_addr >= spi_nor.chip_size:
        log.error('Specified address range is outside of the chip size')
        return 1

    if args.spi_read:
        with open(args.file, mode='bw') as f:
            if args.no_hardware:
                log.warning('No hardware access, operation ignored')
            else:
                with Serial(args.port) as s:
                    s.rts = False
                    rlen = SERIAL_SPI_BLOCK_SIZE - 4
                    tail = (spi_end_addr - spi_start_addr + 1) % rlen
                    rnum = (spi_end_addr - spi_start_addr + 1) // rlen
                    for p in range(rnum):
                        b = spi_read(s, p * rlen, rlen)
                        f.write(bytearray(b))
                    if tail > 0:
                        if rnum > 0:
                            b = spi_read(s, rnum * rlen, tail)
                        else:
                            b = spi_read(s, spi_start_addr, tail)
                        f.write(bytearray(b))

    return 0

if __name__ == '__main__':
    sys.exit(main())
