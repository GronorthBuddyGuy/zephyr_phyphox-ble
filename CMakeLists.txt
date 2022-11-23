cmake_minimum_required(VERSION 3.20)
zephyr_library_named(phyphox_ble)


zephyr_library_sources(src/phyphox_ble.cpp)

target_include_directories(app PUBLIC include)
zephyr_include_directories(include)
