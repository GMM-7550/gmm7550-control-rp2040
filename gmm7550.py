import sys
import asyncio

import board
import digitalio
import busio
import supervisor

from time import sleep

class pin:
    # Green LED controlled directly by RP2040
    led = digitalio.DigitalInOut(board.GP25) # GP0 ?
    led.direction = digitalio.Direction.OUTPUT
    led.value = 0

    # Enable load switch (input power to the FPGA module)
    pwr_en = digitalio.DigitalInOut(board.GP1)
    pwr_en.direction = digitalio.Direction.OUTPUT
    pwr_en.value = 0

    # Disable step-down DC-DCs on the module
    off = digitalio.DigitalInOut(board.GP14)
    off.direction = digitalio.Direction.OUTPUT
    off.value = 0

    # Hardware (Master) Reset to the module
    mr = digitalio.DigitalInOut(board.GP15)
    mr.direction = digitalio.Direction.OUTPUT
    mr.value = 0

i2c = busio.I2C(board.GP3, board.GP2)

uart = busio.UART(board.GP12, board.GP13, # tx, rx
                  baudrate = 19200,
                  timeout = 0.1)

spi_cs = digitalio.DigitalInOut(board.GP9)
spi_cs.direction = digitalio.Direction.OUTPUT
spi_cs.value = 1

spi = busio.SPI(board.GP10, board.GP11, board.GP8) # Clk, MOSI, MISO
while not spi.try_lock():
    pass
spi.configure(baudrate = 20000000)
spi.unlock()

class i2c_io:
    addr = 0x74

    def init():
        while not i2c.try_lock():
            pass

        # Pre-set output bytes for P0 and P1
        i2c.writeto(i2c_io.addr, bytes([2, 0xc2]))
        i2c.writeto(i2c_io.addr, bytes([3, 0x00]))

        # Configure output bits
        i2c.writeto(i2c_io.addr, bytes([6, 0xcc]))
        i2c.writeto(i2c_io.addr, bytes([7, 0x00]))

        # Invert CFG_FAILED_N input
        i2c.writeto(i2c_io.addr, bytes([4, 0x04]))

        i2c.unlock()

    def p0(b = None):
        while not i2c.try_lock():
            pass

        if b == None:
            buf = bytearray(1)
            i2c.writeto_then_readfrom(i2c_io.addr, bytes([0]), buf)
            i2c.unlock()
            return buf[0]
        else:
            i2c.writeto(i2c_io.addr, bytes([2, b]))
            i2c.unlock()

    def p1(b = None):
        while not i2c.try_lock():
            pass

        if b == None:
            buf = bytearray(1)
            i2c.writeto_then_readfrom(i2c_io.addr, bytes([1]), buf)
            i2c.unlock()
            return buf[0]
        else:
            i2c.writeto(i2c_io.addr, bytes([3, b]))
            i2c.unlock()

class SPI_MODE:
    ACTIVE, PASSIVE, FLASH = range(3)

def config (spi_mode = SPI_MODE.ACTIVE, uart = False):
    cfg_mode = 0
    mux_cfg = 0

    if spi_mode == SPI_MODE.ACTIVE:
        pass
    elif spi_mode == SPI_MODE.PASSIVE:
        cfg_mode = 4
        mux_cfg = 1
    elif spi_mode == SPI_MODE.FLASH:
        cfg_mode = 4
        mux_cfg = 2

    if uart :
        mux_cfg |= 4

    b = mux_cfg << 4 | cfg_mode
    # print("P1 <= %02x" % b)
    i2c_io.p1(b)

def on(spi_mode = SPI_MODE.ACTIVE, uart = False):
    pin.off.value = 0
    pin.mr.value = 1
    pin.pwr_en.value = 1
    sleep(.5)
    pin.mr.value = 0
    sleep(.1)
    i2c_io.init()
    config(spi_mode, uart)
    srst(0)

def off():
    pin.pwr_en.value = 0

def hrst(mode = 2):
    if mode == 0:
        pin.mr.value = 0
        sleep(.1)
        i2c_io.init()
    elif mode == 1:
        pin.mr.value = 1
    else:
        pin.mr.value = 1
        sleep(.1)
        pin.mr.value = 0
        sleep(.2)
        i2c_io.init()

def srst(mode = 2):
    b = i2c_io.p0()
    if mode == 0:
        i2c_io.p0(b | 0x01)
    elif mode == 1:
        i2c_io.p0(b & ~0x01)
    else:
        i2c_io.p0(b & ~0x01)
        sleep(.1)
        i2c_io.p0(b | 0x01)

def print_spi_nor_id():
    while not spi.try_lock():
        pass

    id = bytearray(4)

    spi_cs.value = 0
    spi.write_readinto(bytes([0x9f, 0, 0, 0]), id)
    spi_cs.value = 1
    print('SPI Device Info:')
    print('JEDEC ID')
    print('  manufacturer: %02x' % id[1])
    print('   memory type: %02x' % id[2])
    print('      capacity: %02x' % id[3])

    uid = bytearray(4 + 16)
    uid_xfer = [0 for i in range(3 + 16)]
    uid_xfer.insert(0, 0x4b)
    spi_cs.value = 0
    spi.write_readinto(bytes(uid_xfer), uid)
    spi_cs.value = 1

    uid_hex = 'UID:'
    for i in range(16):
        uid_hex += " %02x" % uid[4:][i]
    print(uid_hex)

    spi.unlock()

def cfg_spi_passive():
    # Configure FPGA via SPI
    buf = bytearray(128)

    while not spi.try_lock():
        pass

    with open('bit/led8.bit', 'rb') as cfg_file:
        on(spi_mode = SPI_MODE.PASSIVE)
        l = cfg_file.readinto(buf)
        spi_cs.value = 0
        while l > 0:
            spi.write(buf, end = l)
            l = cfg_file.readinto(buf)
        spi_cs.value = 1

    spi.unlock()
    sleep(5)
    off()

async def uart2stdout():
    while True:
        line = uart.read(16)
        if line:
            sys.stdout.write(line.decode())
        await asyncio.sleep(0)

async def stdin2uart():
    while True:
        cnt = supervisor.runtime.serial_bytes_available
        if cnt > 0:
            data = sys.stdin.read(cnt)
            if data:
                uart.write(data)
        await asyncio.sleep(0)

async def uart_bridge():
    u2s = asyncio.create_task(uart2stdout())
    s2u = asyncio.create_task(stdin2uart())
    await asyncio.gather(s2u, u2s)

def boot_with_uart():
    on(uart = True)
    try:
        asyncio.run(uart_bridge())
    except KeyboardInterrupt:
        pass
    finally:
        off()

def main():
    print('')
    print('GMM-7550 Control')
    print('')

    if True:
        boot_with_uart()
    else:
        cfg_spi_passive()

    # # Read SPI NOR ID
    # on(spi_mode = SPI_MODE.FLASH)
    # print_spi_nor_id()
    # off()
