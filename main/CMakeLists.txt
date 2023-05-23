set(srcs
    "main.c"
)

set(reqs
    "my_NMEA2K_lib"
)

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "")
