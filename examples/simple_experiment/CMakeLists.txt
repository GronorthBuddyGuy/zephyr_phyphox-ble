# Copyright 2022, Victor Chavez
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20.0)

# Path were experiment is located
set(PHYPHOX_EXP  ${CMAKE_CURRENT_SOURCE_DIR}/experiment.phyphox CACHE STRING INTERNAL)
set(PHYPHOX_BLE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Add the pyphoxble repo as a module for its use with zephyr
set(ZEPHYR_EXTRA_MODULES ${PHYPHOX_BLE_ROOT})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(simple_phyphox_experiment)
target_sources(app PRIVATE src/main.cpp
                            src/demo_service.cpp
                )
