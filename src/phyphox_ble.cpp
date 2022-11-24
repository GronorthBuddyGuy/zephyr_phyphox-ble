/*!*****************************************************************
* Copyright 2022, Victor Chavez
* SPDX-License-Identifier: GPL-3.0-or-later
* @file phyphox_ble.cpp
* @author Victor Chavez (chavez-bermudez@fh-aachen.de)
* @date Nov 23, 2022
*
* @brief
* Phyphox ble zephyr implementation
*
* @par Dependencies
* - language: C++17
* - OS: Zephyr
********************************************************************/
#include <cstdint>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>
#include <phyphox_autogen.hpp>
#include <phyphox_ble.hpp>

namespace phyphox_ble
{

static constexpr uint8_t MAX_PACKET_EXP_XML{100U};
static uint16_t exp_xml_data_count{0U};
static uint16_t exp_xml_next_idx{0U};
static bool header_sent{false};

/*!< Offsets for Phyphox raw event data*/
namespace offset
{
    static constexpr uint8_t evt = 0;
    static constexpr uint8_t exptime = 1;
    static constexpr uint8_t unixtime = 9;
} // namespace offset

namespace experiment
{

namespace event
{
    static event_cb user_cb{nullptr};
    static void * user_args{nullptr};
} // namespace event

namespace load
{
    static experiment::load_cb user_cb{nullptr};
    static void * user_args{nullptr};
} // namespace exp_load_cb

bool set_title(char * pData, uint8_t len)
{
    static constexpr uint16_t MAX_TITLE_LEN{autogen::EXP_TITLE_END - autogen::EXP_TITLE_START};
    if(pData!=nullptr && len<=MAX_TITLE_LEN)
    {
        memcpy(&autogen::exp_data[autogen::EXP_TITLE_START], pData, len);
        if(len<MAX_TITLE_LEN)
        {
            const uint16_t remaining = MAX_TITLE_LEN-len;
            memset(&autogen::exp_data[autogen::EXP_TITLE_START+len],
                    static_cast<int>(' '),
                    remaining);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool set_blename_in(char * pData, uint8_t len)
{
    if(autogen::BLE_END_NAME_IN == static_cast<uint16_t>(-1))
    {
        return false;
    }
    if(autogen::BLE_END_NAME_IN<=autogen::BLE_START_NAME_IN)
    {
        return false;
    }
    static constexpr uint16_t MAX_NAME_LEN = autogen::BLE_END_NAME_IN - autogen::BLE_START_NAME_IN;
    if(pData!=nullptr && len<=MAX_NAME_LEN)
    {
        memcpy(&autogen::exp_data[autogen::BLE_START_NAME_IN], pData, len);
        if(len<MAX_NAME_LEN)
        {
            const uint16_t remaining = MAX_NAME_LEN-len;
            autogen::exp_data[autogen::BLE_START_NAME_IN+len]='"';
            if(remaining > 1)
            {
                memset(&autogen::exp_data[autogen::BLE_START_NAME_IN+len+1],
                        static_cast<int>(' '),
                        remaining-1);
            }
            autogen::exp_data[autogen::BLE_END_NAME_IN]=' ';
            
        }
        return true;
    }
    else
    {
        return false;
    }
}

void update_crc()
{
    const uint32_t crc = crc32_ieee(autogen::exp_data, sizeof(autogen::exp_data));
    const uint32_t crc_be = sys_be32_to_cpu(crc);
    uint8_t * header_start = &autogen::exp_header[autogen::HEADER_SIZE-sizeof(uint32_t)];
    memcpy(header_start, &crc_be, sizeof(uint32_t));
}

} // namespace experiment


/*! @brief send a chunk of the complete xml to phyphox
*/
static void send_exp_xml(); 

/*! @brief Callback to sync sending the complete xml file
    over ble notification
*/
void exp_xml_notify_sent_cb(struct bt_conn *conn, void *user_data)
{
    static_cast<void>(conn);
    static_cast<void>(user_data);
    // continue sending xml if still data pending
    if(exp_xml_data_count< autogen::EXP_DATA_SIZE)
    {
        send_exp_xml();
    }
}

namespace experiment
{
    void register_load_cb(load_cb cb, void * args)
    {
        load::user_cb = cb;
        load::user_args = args;
    }

    void register_evt_cb(event_cb cb, void * args)
    {
        event::user_cb = cb;
        event::user_args = args;
    }
} // namespace experiment

static void exp_xml_notify(const struct bt_gatt_attr *attr, uint16_t value)
{
    static_cast<void>(attr);
    const bool notify_enabled = value == BT_GATT_CCC_NOTIFY;
    if(notify_enabled)
    {
        exp_xml_data_count = 0;
        exp_xml_next_idx=0;
        header_sent = false;
        send_exp_xml();
    }
}

static ssize_t eventwrite_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
    static_cast<void>(conn);
    static_cast<void>(attr);
    static_cast<void>(offset);
    static_cast<void>(flags);
    const auto pBufRaw = static_cast<const uint8_t*>(buf);
    /* convert big endian to little endian
    */
    uint64_t exp_time_ms{0};
    uint64_t unixtime_ms{0};
    memcpy(&exp_time_ms, &pBufRaw[offset::exptime], sizeof(exp_time_ms));
    memcpy(&unixtime_ms, &pBufRaw[offset::unixtime], sizeof(unixtime_ms));
    exp_time_ms = sys_cpu_to_be64(exp_time_ms);
    unixtime_ms = sys_cpu_to_be64(unixtime_ms);
    const experiment::Event_t event_data =
    {
        .evt_type = static_cast<experiment::EventTypes>(pBufRaw[0]),
        .exp_time_ms = exp_time_ms,
        .unix_time_ms = unixtime_ms
    };
    
    if(experiment::event::user_cb != nullptr)
    {
        experiment::event::user_cb(event_data, experiment::event::user_args);
    }
	return len;
}


constexpr uint8_t exp_xml_char_idx{1U};

BT_GATT_SERVICE_DEFINE(phy_phox_svc,
	BT_GATT_PRIMARY_SERVICE( (void *)&uuid::SERVICE), // idx 0
	BT_GATT_CHARACTERISTIC( (bt_uuid *)&uuid::charact::EXP_XML,  // idx 1
							BT_GATT_CHRC_NOTIFY,
							BT_GATT_PERM_READ,
							nullptr,
							nullptr,
							nullptr),
	BT_GATT_CCC(exp_xml_notify, // idx 2
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), // cppcheck-suppress syntaxError
	BT_GATT_CHARACTERISTIC( (bt_uuid *)&uuid::charact::EVENT,  // idx 3
							BT_GATT_CHRC_WRITE,
							BT_GATT_PERM_WRITE,
							nullptr,
							eventwrite_cb,
							nullptr),
);


static void send_exp_xml()
{
    bt_gatt_notify_params exp_notify_params =
    {
        .uuid = nullptr,
        .attr = &phy_phox_svc.attrs[exp_xml_char_idx],
        .data = nullptr,
        .len = 0,
        .func = exp_xml_notify_sent_cb,
        .user_data = nullptr
    };
    bool finished_upload{false};
    /* First packet must be phyphox header*/
    if(header_sent == false)
    {
        header_sent = true;
        exp_notify_params.data = &autogen::exp_header[0];
        exp_notify_params.len = autogen::HEADER_SIZE;
    }
    else
    {
        exp_xml_data_count+=MAX_PACKET_EXP_XML;
        uint8_t send_len{MAX_PACKET_EXP_XML};
        if(exp_xml_data_count > autogen::EXP_DATA_SIZE)
        {
            const uint16_t extra = exp_xml_data_count- autogen::EXP_DATA_SIZE;
            send_len -=extra;
            finished_upload = true;
        }
        exp_notify_params.data = &autogen::exp_data[exp_xml_next_idx];
        exp_notify_params.len = send_len;
        exp_xml_next_idx += send_len;
    }
    const int gatt_res = bt_gatt_notify_cb(nullptr, &exp_notify_params);
    __ASSERT(gatt_res==0, "Could not start notification of experiment, check MTU Size");
    if(finished_upload)
    {
        if(experiment::load::user_cb != nullptr)
        {
            experiment::load::user_cb(experiment::load::user_args);
        }
    }
}

} // namespace  phyphox_ble
