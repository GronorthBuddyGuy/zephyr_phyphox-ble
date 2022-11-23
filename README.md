# Zephyr Phyphox BLE Module

Zephyr module that enables the integration of the [Phyphox](https://phyphox.org/) app to transmit data over its BLE [custom service](https://phyphox.org/wiki/index.php/Bluetooth_Low_Energy).

## Requirements

- Zephyr OS v3.x
- Python 3
- C++ >=17


### BLE

This module allows to easily integrate the phyphox ble service. This service is mainly used to transmit the phyphox experiment and get data from events that occur during the experiment. When using this module be sure to allow at least MTU size of 100 bytes. Further information about the API is found in includes/phyphox_ble.hpp or in the examples directory.

### Installation



