/*!*****************************************************************
* Copyright 2022, Victor Chavez
* SPDX-License-Identifier: GPL-3.0-or-later
* @file main.cpp
* @author Victor Chavez (chavez-bermudez@fh-aachen.de)
* @date Nov 23, 2022
*
* @brief
* Demo application that shows a simple experiment integration with phyphox module
* @par Dependencies
* - language: C++17
* - OS: Zephyr
********************************************************************/
#include <phyphox_ble.hpp>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/random/rand32.h>
#include <assert.h>
#include "demo_service.hpp"

static constexpr uint32_t loop_delay_ms{500};

static void onConnect(bt_conn *conn, uint8_t err);

/*! @brief Callbacks for bt connection.
*/
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = onConnect,
	.disconnected = nullptr,
    .le_param_req = nullptr,
    .le_param_updated = nullptr
};

namespace adv
{
static constexpr
bt_data data[] = 
{
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                    RAW_UUID_128(phyphox_ble::uuid::SERVICE)),
};

static constexpr
bt_le_adv_param params = BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONNECTABLE |
                                                BT_LE_ADV_OPT_USE_NAME,
                                                BT_GAP_PER_ADV_SLOW_INT_MIN,
                                                BT_GAP_PER_ADV_SLOW_INT_MAX,
                                                NULL);
void start()
{
    const int err = bt_le_adv_start(&params,
                        data,
                        ARRAY_SIZE(data),
                        nullptr,
                        0);
    __ASSERT(err==0,"Adv start failed %d",err);
}

} // namespace adv

void ble_init()
{
    int res{0U};
    do
    {
        res =  bt_enable(nullptr);
        if(res != 0)
        {
            break;
        }
        res =  bt_set_name("Zephyr Phyphox Demo");
        if(res != 0)
        {
            break;
        }
    } while(false);
    __ASSERT(res == 0,"Could not start BLE Peripheral");
}

/*!< Callback for when an MTU exchange is done*/
static void mtuexchange_cb(bt_conn *conn, uint8_t err,
		                    bt_gatt_exchange_params *params)
{
    static_cast<void>(conn);
    static_cast<void>(err);
    static_cast<void>(params);
    printk("MTU exchang done\n");
}


static void onConnect(bt_conn *conn, uint8_t err)
{
    if(err == 0)
    {
        static bt_gatt_exchange_params exchange_params
        {
            .func = mtuexchange_cb
        };
        /*<! MTU size increase to send correctly phyphox experiment*/
        const int mtu_req = bt_gatt_exchange_mtu(conn, &exchange_params);
        __ASSERT(mtu_req==0,"Could not set mtu exchange");
    }
}

int main(void)
{
    ble_init();
    adv::start();
    while(true)
    {
        demo_service::notify_uptime();
        const auto rand_val = sys_rand32_get();
        demo_service::notify_simple_value(rand_val);
        k_sleep(K_MSEC(loop_delay_ms));
    }
    return 0;
}
