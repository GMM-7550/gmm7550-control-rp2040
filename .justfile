# build directory name
b := "build"

# rebuild everything from scratch
do: clean cmake make

# remove build directory
clean:
  rm -rf {{b}}

# run cmake
cmake:
  cmake -B {{b}}

# run make
make:
  make -C {{b}}

# copy uf2 image to Raspberry Pi
cp:
  cp {{b}}/gmm_control.uf2 /run/media/ak/RPI-RP2/

default_baud := "115200"
# connect to the target (GMM-7550) serial interface
com baud=default_baud:
  picocom -q -b {{baud}} /dev/ttyACM0

# connect to RP2040 CLI (control interface)
cli:
  picocom -q -b 115200 /dev/ttyACM1

# download bit-file to GMM-7550 via SPI interface (cfg 4; mux 1)
cfg bitFile:
  ./tools/gmm7550_spi.py --configure {{bitFile}}

dmesg:
  sudo dmesg | grep -i usb | tail -20

lsusb:
  sudo lsusb -d cafe: -v
