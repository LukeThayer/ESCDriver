
cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()





add_executable(main
        main.c
        )


pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

# pull in common dependencies
# target_link_libraries(main pico_stdlib
#    pico_cyw43_arch_none )

# create map/bin/hex file etc.
pico_add_extra_outputs(main)



target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries(main pico_cyw43_arch_lwip_threadsafe_background pico_stdlib hardware_pwm)

