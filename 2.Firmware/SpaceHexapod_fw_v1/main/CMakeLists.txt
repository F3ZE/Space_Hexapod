#idf_component_register(SRCS "hello_world_main.c"
#                    PRIV_REQUIRES spi_flash
#                    INCLUDE_DIRS "")
idf_component_register(SRCS "main.c"  # 源文件
        INCLUDE_DIRS ".")  # 包含当前目录

target_compile_options(${COMPONENT_LIB} PRIVATE "-finput-charset=UTF-8" "-fexec-charset=UTF-8")
