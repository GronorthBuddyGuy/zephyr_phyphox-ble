/*!*****************************************************************
* Copyright 2022, Victor Chavez
* SPDX-License-Identifier: GPL-3.0-or-later
* @file demo_service.hpp
* @author Victor Chavez (chavez-bermudez@fh-aachen.de)
* @date Nov 23, 2022
*
* @brief
* Demo service that allows to notify ble data to phyphox experiment
* @par Dependencies
* - language: C++17
* - OS: Zephyr
********************************************************************/
#pragma once
#include <cstdint>

namespace demo_service
{
    /*!@brief BLE Notify zephyr uptime */
    void notify_uptime();
    /*!@brief BLE Notify a simple uint32_t value */
    void notify_simple_value(uint32_t value);
}