set(srcs
    "my_N2K_lib.cpp"
    "N2kGroupFunctionBinaryStatus.cpp"
    "ESP32N2kStream.cpp"
)

set(reqs
    "NMEA2000"
    "NMEA2000_esp32"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES ${reqs}
)