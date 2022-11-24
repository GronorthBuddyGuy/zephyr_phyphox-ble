/*!*****************************************************************
* Copyright 2022, Victor Chavez
* SPDX-License-Identifier: GPL-3.0-or-later
* @file demo_service.cpp
* @author Victor Chavez (chavez-bermudez@fh-aachen.de)
* @date Nov 23, 2022
*
* @par Dependencies
* - language: C++17
* - OS: Zephyr
********************************************************************/
#include <zephyr/bluetooth/gatt.h>
#include <uuid_utils.hpp>
#include "demo_service.hpp"

namespace demo_service
{
    static float uptime{0.0f};
    static uint32_t simple_val{0};
    namespace uuid
    {
        /*! Arbitriary UUID for demo service */
        static constexpr bt_uuid_128 BASE = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12340000, 
                                                                    0x0000,
                                                                    0x0000,
                                                                    0x0000,
                                                                    0x1234567859AB));
        /*! Service UUID */
        static constexpr bt_uuid_128 SERVICE = ble::utils::uuid::derive_uuid(BASE,0x0001);
        /*! characteristic UUIDs */
        namespace charact
        {
            static constexpr bt_uuid_128 UPTIME= ble::utils::uuid::derive_uuid(BASE,0x0002);
            static constexpr bt_uuid_128 SIMPLE_VAL = ble::utils::uuid::derive_uuid(BASE,0x0003);
        }
    } // namespace uuid

    namespace attr_idx
    {
        constexpr uint8_t uptime{2U};
        constexpr uint8_t simple_val{4U};
    } // namespace attr_idx

    BT_GATT_SERVICE_DEFINE(demo_svc,
        BT_GATT_PRIMARY_SERVICE( (void *)&uuid::SERVICE), // idx 0
        BT_GATT_CHARACTERISTIC( (bt_uuid *)&uuid::charact::UPTIME,  // idx 1
                                BT_GATT_CHRC_NOTIFY,
                                BT_GATT_PERM_READ,
                                nullptr,
                                nullptr,
                                nullptr),
        BT_GATT_CCC(nullptr, // idx 2
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), // cppcheck-suppress syntaxError
        BT_GATT_CHARACTERISTIC( (bt_uuid *)&uuid::charact::SIMPLE_VAL,  // idx 3
                                BT_GATT_CHRC_NOTIFY,
                                BT_GATT_PERM_READ,
                                nullptr,
                                nullptr,
                                nullptr),
        BT_GATT_CCC(nullptr, // idx 4
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), // cppcheck-suppress syntaxError
    );

    void notify_uptime()
    {
        static constexpr float ms_to_s{1000.0f};
        const uint32_t uptime_ms =  k_uptime_get_32();
        uptime = static_cast<float>(uptime_ms)/ms_to_s;
        const int gatt_res = bt_gatt_notify(nullptr,
										&demo_svc.attrs[attr_idx::uptime],
										&uptime,
										sizeof(uptime));
        static_cast<void>(gatt_res);
    }

    void notify_simple_value(uint32_t value)
    {
        simple_val = value;
        const int gatt_res = bt_gatt_notify(nullptr,
										&demo_svc.attrs[attr_idx::simple_val],
										&simple_val,
										sizeof(simple_val));
        static_cast<void>(gatt_res);
    }
} // namespace demo_service
