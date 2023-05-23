| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- |

# NMEA2000_switchbank_example_esp-idf

Example of using [NMEA2000](https://github.com/ttlappalainen/NMEA2000) library in an ESP-IDF project.  

The NMEA2000 library is typically used in the Arduino environment, but it can be used without Arduino.  Documentation was lacking on how to do this, so I created this example to demonstrate how it can be done in the ESP-IDF (4.4+) environment.

This example also demonstrates a simple "Binary Switchbank" device on the NMEA2000 bus.  (I.e. a bank of relay-switches which can be discovered and controlled by a marine display/chartplotter.)

## How to use this example

Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)

Clone this repo and make sure to get the submodules.
`git clone https://github.com/phatpaul/NMEA2000_switchbank_example_esp-idf.git --recursive`

(If you forgot the --recusive clone, then you can get the submodules later with `git submodule update --init --recursive`)

Launch the ESP-IDF build environment (varies with OS).  Then `idf build`

## Example project structure

The [NMEA2000](https://github.com/ttlappalainen/NMEA2000) and [NMEA2000_esp32](https://github.com/ttlappalainen/NMEA2000_esp32) repos are located in components/ dir of this project as git submodules.

ESP-IDF app code is typically written in C.  So to call C++ library functions, I created a wrapper called `my_N2K_lib` as another component.
```
my-esp-idf-project/
  - components/
    - NMEA2000/
    - NMEA2000_esp32/
    - my_N2K_lib/
      - CMakeLists.txt
      - my_N2K_lib.cpp
      - my_N2K_lib.h
  - main/
    - CMakeLists.txt
    - main.c
  - CMakeLists.txt 
```