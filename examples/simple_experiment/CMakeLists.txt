cmake_minimum_required(VERSION 3.20.0)


set(PHYPHOX_BLE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Add the pyphoxble repo as a modul for its use with zephyr
set(ZEPHYR_EXTRA_MODULES ${PHYPHOX_BLE_ROOT})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_sources(app PRIVATE src/main.cpp)
