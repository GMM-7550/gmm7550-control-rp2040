# Control GMM-7550 from Raspberry RP2040 Microcontroller

RP2040 firmware to control GMM-7550 module connected to the
USB3 adapter boards.

## Build instructions

Getting sources (note that FreeRTOS Kernel and Raspberry Pi
Pico SDK are included as submodules)

```
    git clone https://github.com/GMM-7550/gmm7550-control-rp2040.git
    cd gmm7550-control-rp2040
    git submodule update --init --recursive
```

Building firmware and programming it to the board

```
    cmake -B build
    make -C build
    cp build/gmm_control.uf2 <mount-point>/RPI-RP2/
```
