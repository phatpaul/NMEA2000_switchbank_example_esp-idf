set(srcs
    "main.c"
)

set(reqs
    "my_N2K_lib"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ""
    REQUIRES ${reqs}
)